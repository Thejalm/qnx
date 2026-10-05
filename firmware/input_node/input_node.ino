/*
 * ==============================================================================
 * Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
 * Component: ESP32-C3 Input Node (Sensors Acquisition & Telemetry Framing)
 * Board: ESP32-C3 (16-Pin DevModule / SuperMini)
 * 
 * Sensors Connected:
 * 1. BME280 (I2C: Temperature, Humidity, Barometric Pressure)
 * 2. MQ-2 Gas/Smoke Sensor (Analog ADC + Digital Threshold)
 * 3. Flame Sensor (Digital Detection + Analog Intensity)
 * 
 * Architecture Role:
 * - Deterministic non-blocking sampling (10 Hz / 100 ms)
 * - Hardware fault detection (sensor disconnect / out-of-range detection)
 * - Structured packet framing with sequence counter & XOR-8 Checksum
 * - Wired USB-Serial telemetry output to Node Computer
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <Adafruit_BMP280.h>

// ============================================================================
// CONFIGURATION & PIN DEFINITIONS (ESP32-C3 16-Pin)
// ============================================================================
#define NODE_ID             1
#define FIRMWARE_VERSION    "1.1.0"
#define SERIAL_BAUD_RATE    115200
#define SAMPLING_PERIOD_MS  100    // 10 Hz deterministic acquisition rate

// I2C Pins for BME280 / BMP280
#define PIN_I2C_SDA         4
#define PIN_I2C_SCL         5

// Analog & Digital Pins for MQ-2 Gas Sensor
#define PIN_MQ2_ANALOG      0      // ADC1_CH0
#define PIN_MQ2_DIGITAL     1      // Digital threshold output (Active LOW on detection)

// Pins for Flame Sensor
#define PIN_FLAME_DIGITAL   2      // Digital threshold (Active LOW on flame trigger)
#define PIN_FLAME_ANALOG    3      // ADC1_CH3 (Flame IR intensity level)

// Onboard Status LED (ESP32-C3 typically has onboard LED at GPIO 8)
#define PIN_STATUS_LED      8

// ============================================================================
// GLOBAL OBJECTS & STATE VARIABLES
// ============================================================================
Adafruit_BME280 bme; // I2C BME280 instance
Adafruit_BMP280 bmp; // I2C BMP280 instance (fallback if chip is BMP280)
bool bme_detected = false;
bool bmp_detected = false;

uint32_t sequence_number = 0;
unsigned long last_sample_time = 0;
unsigned long last_blink_time = 0;
bool status_led_state = false;

// Sensor data structure
struct SensorData {
    float temperature;      // deg C
    float humidity;         // %
    float pressure;         // hPa
    int mq2_raw_adc;        // 0 - 4095
    bool mq2_digital_alert; // 1 = gas threshold exceeded
    bool flame_detected;    // 1 = flame detected
    int flame_raw_adc;      // 0 - 4095 (lower value = stronger IR flame signal)
    uint8_t fault_flags;    // Bitmask: bit 0: BME fault, bit 1: MQ2 open/short, bit 2: Flame fault
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Calculates XOR-8 checksum over a buffer string
 */
uint8_t calculate_xor_checksum(const char* data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= (uint8_t)data[i];
    }
    return checksum;
}

/**
 * Reads and validates all sensors
 */
void acquire_sensors(SensorData &data) {
    data.fault_flags = 0x00;

    // 1. Read BME280 or BMP280
    if (bme_detected) {
        data.temperature = bme.readTemperature();
        data.humidity    = bme.readHumidity();
        data.pressure    = bme.readPressure() / 100.0F; // Convert Pa to hPa

        if (isnan(data.temperature) || isnan(data.humidity) || isnan(data.pressure) ||
            data.temperature < -40.0f || data.temperature > 85.0f) {
            data.fault_flags |= (1 << 0);
            data.temperature = -999.0f;
            data.humidity    = -999.0f;
            data.pressure    = -999.0f;
        }
    } else if (bmp_detected) {
        data.temperature = bmp.readTemperature();
        data.humidity    = 50.0f; // BMP280 does not have humidity; default to nominal 50%
        data.pressure    = bmp.readPressure() / 100.0F;

        if (isnan(data.temperature) || isnan(data.pressure) ||
            data.temperature < -40.0f || data.temperature > 85.0f) {
            data.fault_flags |= (1 << 0);
            data.temperature = -999.0f;
            data.humidity    = -999.0f;
            data.pressure    = -999.0f;
        }
    } else {
        data.fault_flags |= (1 << 0);
        data.temperature = -999.0f;
        data.humidity    = -999.0f;
        data.pressure    = -999.0f;
    }

    // 2. Read MQ-2 Gas Sensor
    data.mq2_raw_adc = analogRead(PIN_MQ2_ANALOG);
    data.mq2_digital_alert = (digitalRead(PIN_MQ2_DIGITAL) == LOW);
    
    if (data.mq2_raw_adc < 5 || data.mq2_raw_adc > 4090) {
        data.fault_flags |= (1 << 1);
    }

    // 3. Read Flame Sensor
    data.flame_detected = (digitalRead(PIN_FLAME_DIGITAL) == LOW);
    data.flame_raw_adc  = analogRead(PIN_FLAME_ANALOG);
}

/**
 * Builds and transmits the telemetry packet over USB/Serial:
 */
void send_telemetry_packet(const SensorData &data, unsigned long timestamp_ms) {
    char payload[180];
    char full_packet[200];

    snprintf(payload, sizeof(payload),
             "IN,%u,%lu,%lu,%.2f,%.2f,%.2f,%d,%d,%d,%d,%u",
             NODE_ID,
             sequence_number,
             timestamp_ms,
             data.temperature,
             data.humidity,
             data.pressure,
             data.mq2_raw_adc,
             data.mq2_digital_alert ? 1 : 0,
             data.flame_detected ? 1 : 0,
             data.flame_raw_adc,
             data.fault_flags);

    uint8_t checksum = calculate_xor_checksum(payload, strlen(payload));
    snprintf(full_packet, sizeof(full_packet), "$%s*%02X\r\n", payload, checksum);
    Serial.print(full_packet);

    sequence_number++;
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    pinMode(PIN_MQ2_DIGITAL, INPUT_PULLUP);
    pinMode(PIN_FLAME_DIGITAL, INPUT_PULLUP);
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    analogReadResolution(12);

    // Initialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    delay(100);

    // Try BME280 at 0x76 & 0x77
    if (bme.begin(0x76, &Wire)) {
        bme_detected = true;
    } else if (bme.begin(0x77, &Wire)) {
        bme_detected = true;
    } 
    // Fallback: Try BMP280 at 0x76 & 0x77
    else if (bmp.begin(0x76)) {
        bmp_detected = true;
    } else if (bmp.begin(0x77)) {
        bmp_detected = true;
    }

    if (bme_detected) {
        bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                        Adafruit_BME280::SAMPLING_X2,
                        Adafruit_BME280::SAMPLING_X16,
                        Adafruit_BME280::SAMPLING_X1,
                        Adafruit_BME280::FILTER_X16,
                        Adafruit_BME280::STANDBY_MS_0_5);
    }
}

// ============================================================================
// ARDUINO MAIN LOOP
// ============================================================================
void loop() {
    unsigned long current_time = millis();

    // Deterministic 100ms periodic sampling and transmission
    if (current_time - last_sample_time >= SAMPLING_PERIOD_MS) {
        last_sample_time = current_time;

        SensorData data;
        acquire_sensors(data);
        send_telemetry_packet(data, current_time);
    }

    // Heartbeat visual indicator (toggle status LED every 500 ms)
    if (current_time - last_blink_time >= 500) {
        last_blink_time = current_time;
        status_led_state = !status_led_state;
        digitalWrite(PIN_STATUS_LED, status_led_state ? HIGH : LOW);
    }
}
