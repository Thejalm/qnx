/*
 * ==============================================================================
 * Project: QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator
 * Component: ESP32-C3 Output Actuator & Display Node
 * Board: ESP32-C3 (16-Pin DevModule / SuperMini)
 * 
 * Actuators & Peripherals Connected:
 * 1. Dual Active Buzzers (Alarm & Alert)
 * 2. 2-Channel Relay Module (Relay 1: Exhaust Fan, Relay 2: Water Pump)
 * 3. 0.96" I2C OLED Display (SSD1306 128x64)
 * 4. Discrete Status LEDs (Yellow = Warning, Red = Critical/Fault)
 * 
 * Architecture Role:
 * - Listens for verified serial commands from QNX Master / Node Controller
 * - Executes deterministic actuator control with hardware fail-safe watchdog
 * - Displays real-time parameters, health state, and safety status on OLED
 * - Transmits acknowledgment (ACK) packets with exact sequence matching
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================================
// CONFIGURATION & PIN DEFINITIONS (ESP32-C3 16-Pin)
// ============================================================================
#define NODE_ID             2
#define FIRMWARE_VERSION    "1.0.0"
#define SERIAL_BAUD_RATE    115200

// OLED Display Parameters
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_RESET          -1
#define SCREEN_I2C_ADDR     0x3C

// I2C Pins for SSD1306 OLED
#define PIN_I2C_SDA         4
#define PIN_I2C_SCL         5

// Active Buzzers
#define PIN_BUZZER_1        0      // Primary Alarm Buzzer (Continuous/Loud)
#define PIN_BUZZER_2        1      // Secondary Warning Buzzer (Pulsed/Beep)

// 2-Channel Relay Module (Relays are typically Active-LOW)
#define PIN_RELAY_FAN       2      // Relay 1: High-Flow Cooling/Exhaust Fan
#define PIN_RELAY_PUMP      3      // Relay 2: Water Pump / Suppression

// Discrete Status LEDs
#define PIN_LED_YELLOW      6      // Yellow: Warning / Degraded state
#define PIN_LED_RED         7      // Red: Critical Safety Trip / Fault
#define PIN_STATUS_LED      8      // Onboard Heartbeat LED

// Fail-Safe Watchdog Parameters
#define FAILSAFE_TIMEOUT_MS 2000   // Trigger fail-safe if no packet in 2.0s

// ============================================================================
// GLOBAL OBJECTS & STATE
// ============================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oled_initialized = false;

// Actuator States
bool state_buzzer1 = false;
bool state_buzzer2 = false;
bool state_relay_fan = false;
bool state_relay_pump = false;
bool state_led_yellow = false;
bool state_led_red = false;

// Fail-Safe and Tracking Variables
unsigned long last_valid_command_time = 0;
bool failsafe_active = false;
uint32_t last_received_seq = 0;
char last_status_text[32] = "INITIALIZING";

// Serial Receiver Buffer
#define RX_BUFFER_SIZE 256
char rx_buffer[RX_BUFFER_SIZE];
size_t rx_index = 0;

// Display refresh timer
unsigned long last_display_update = 0;
unsigned long last_heartbeat_toggle = 0;
bool heartbeat_state = false;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Calculates XOR-8 checksum over a string buffer
 */
uint8_t calculate_xor_checksum(const char* data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= (uint8_t)data[i];
    }
    return checksum;
}

/**
 * Applies physical states to GPIO hardware
 */
void apply_actuator_hardware_states() {
    // Active Buzzers: HIGH = ON, LOW = OFF
    digitalWrite(PIN_BUZZER_1, state_buzzer1 ? HIGH : LOW);
    digitalWrite(PIN_BUZZER_2, state_buzzer2 ? HIGH : LOW);

    // Relays (Active-LOW: LOW = Energized/ON, HIGH = De-energized/OFF)
    digitalWrite(PIN_RELAY_FAN, state_relay_fan ? LOW : HIGH);
    digitalWrite(PIN_RELAY_PUMP, state_relay_pump ? LOW : HIGH);

    // LEDs: HIGH = ON, LOW = OFF
    digitalWrite(PIN_LED_YELLOW, state_led_yellow ? HIGH : LOW);
    digitalWrite(PIN_LED_RED, state_led_red ? HIGH : LOW);
}

/**
 * Enters hardware fail-safe state when communication with QNX/Node Controller is lost
 */
void enter_failsafe_state() {
    failsafe_active = true;
    state_buzzer1 = true;      // Activate emergency alarm
    state_buzzer2 = false;
    state_relay_fan = true;    // Force exhaust fan ON to prevent heat/gas buildup
    state_relay_pump = false;  // Keep pump OFF unless actively commanded
    state_led_yellow = true;   // Warning ON
    state_led_red = true;      // Critical LED ON
    strncpy(last_status_text, "COMM FAILSAFE!", sizeof(last_status_text));
    apply_actuator_hardware_states();
}

/**
 * Renders the OLED Display dashboard
 */
void update_oled_display() {
    if (!oled_initialized) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Title Bar
    display.setCursor(0, 0);
    display.print("QNX SAFETY ORCHESTRATOR");
    display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

    // Status Banner
    display.setCursor(0, 12);
    display.print("SYS: ");
    if (failsafe_active) {
        display.print("FAILSAFE (LOST LINK)");
    } else {
        display.print(last_status_text);
    }

    // Actuator States (2 Column Grid)
    display.setCursor(0, 24);
    display.printf("FAN : [%s]", state_relay_fan ? "RUN " : "STOP");
    display.setCursor(68, 24);
    display.printf("PUMP: [%s]", state_relay_pump ? "RUN " : "STOP");

    display.setCursor(0, 36);
    display.printf("BUZ1: [%s]", state_buzzer1 ? "ON " : "OFF");
    display.setCursor(68, 36);
    display.printf("BUZ2: [%s]", state_buzzer2 ? "ON " : "OFF");

    display.setCursor(0, 48);
    display.printf("Y-LED:[%s]", state_led_yellow ? "ON " : "OFF");
    display.setCursor(68, 48);
    display.printf("R-LED:[%s]", state_led_red ? "ON " : "OFF");

    // Sequence & Heartbeat Line
    display.setCursor(0, 56);
    display.printf("SEQ:%05lu LINK:%s", last_received_seq, failsafe_active ? "DOWN" : "LIVE");

    display.display();
}

/**
 * Transmits an ACK response packet over Serial
 * Format: $ACK,NODE_ID,SEQ,BUZ1,BUZ2,FAN,PUMP,LED_Y,LED_R,FAILSAFE*CHECKSUM\r\n
 */
void send_ack_packet(uint32_t seq) {
    char payload[128];
    char full_packet[160];

    snprintf(payload, sizeof(payload),
             "ACK,%u,%lu,%d,%d,%d,%d,%d,%d,%d",
             NODE_ID,
             seq,
             state_buzzer1 ? 1 : 0,
             state_buzzer2 ? 1 : 0,
             state_relay_fan ? 1 : 0,
             state_relay_pump ? 1 : 0,
             state_led_yellow ? 1 : 0,
             state_led_red ? 1 : 0,
             failsafe_active ? 1 : 0);

    uint8_t checksum = calculate_xor_checksum(payload, strlen(payload));
    snprintf(full_packet, sizeof(full_packet), "$%s*%02X\r\n", payload, checksum);
    Serial.print(full_packet);
}

/**
 * Parses and executes a verified command packet:
 * Format: $CMD,NODE_ID,SEQ,BUZZER1,BUZZER2,RELAY_FAN,RELAY_PUMP,LED_Y,LED_R,STATUS_STR*CHECKSUM
 */
bool parse_command_packet(char* packet_str) {
    // 1. Verify packet delimiters
    if (packet_str[0] != '$') return false;
    char* star_pos = strchr(packet_str, '*');
    if (!star_pos) return false;

    // Extract Checksum
    *star_pos = '\0';
    const char* payload = &packet_str[1];
    const char* checksum_hex = star_pos + 1;

    uint8_t expected_checksum = (uint8_t)strtol(checksum_hex, NULL, 16);
    uint8_t actual_checksum = calculate_xor_checksum(payload, strlen(payload));

    if (expected_checksum != actual_checksum) {
        return false; // Checksum Mismatch (Corrupted Packet)
    }

    // 2. Tokenize CSV Payload
    char payload_copy[RX_BUFFER_SIZE];
    strncpy(payload_copy, payload, sizeof(payload_copy));
    payload_copy[sizeof(payload_copy) - 1] = '\0';

    char* token = strtok(payload_copy, ",");
    if (!token || strcmp(token, "CMD") != 0) return false;

    // Node ID
    token = strtok(NULL, ",");
    if (!token || atoi(token) != NODE_ID) return false;

    // Sequence Number
    token = strtok(NULL, ",");
    if (!token) return false;
    uint32_t seq = strtoul(token, NULL, 10);

    // Buzzer 1
    token = strtok(NULL, ",");
    if (!token) return false;
    int b1 = atoi(token);

    // Buzzer 2
    token = strtok(NULL, ",");
    if (!token) return false;
    int b2 = atoi(token);

    // Relay Fan
    token = strtok(NULL, ",");
    if (!token) return false;
    int r_fan = atoi(token);

    // Relay Pump
    token = strtok(NULL, ",");
    if (!token) return false;
    int r_pump = atoi(token);

    // Yellow LED
    token = strtok(NULL, ",");
    if (!token) return false;
    int led_y = atoi(token);

    // Red LED
    token = strtok(NULL, ",");
    if (!token) return false;
    int led_r = atoi(token);

    // Status String
    token = strtok(NULL, ",");
    if (token) {
        strncpy(last_status_text, token, sizeof(last_status_text) - 1);
        last_status_text[sizeof(last_status_text) - 1] = '\0';
    }

    // 3. Commit state changes
    state_buzzer1    = (b1 != 0);
    state_buzzer2    = (b2 != 0);
    state_relay_fan  = (r_fan != 0);
    state_relay_pump = (r_pump != 0);
    state_led_yellow = (led_y != 0);
    state_led_red    = (led_r != 0);

    last_received_seq = seq;
    last_valid_command_time = millis();
    failsafe_active = false;

    apply_actuator_hardware_states();
    send_ack_packet(seq);

    return true;
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    // Wait for USB CDC Serial to connect (up to 3 seconds)
    unsigned long start_wait = millis();
    while(!Serial && (millis() - start_wait < 3000)) {
        delay(10);
    }
    Serial.println("\n[DEBUG] Serial initialized.");
    delay(100);

    // 1. Configure Actuator Pins as Outputs
    pinMode(PIN_BUZZER_1, OUTPUT);
    pinMode(PIN_BUZZER_2, OUTPUT);
    pinMode(PIN_RELAY_FAN, OUTPUT);
    pinMode(PIN_RELAY_PUMP, OUTPUT);
    pinMode(PIN_LED_YELLOW, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);

    // 2. Set safe initial default states (All Actuators OFF, Relays De-energized)
    digitalWrite(PIN_BUZZER_1, LOW);
    digitalWrite(PIN_BUZZER_2, LOW);
    digitalWrite(PIN_RELAY_FAN, HIGH);  // Active-LOW: HIGH = Relay OFF
    digitalWrite(PIN_RELAY_PUMP, HIGH); // Active-LOW: HIGH = Relay OFF
    digitalWrite(PIN_LED_YELLOW, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_STATUS_LED, LOW);

    Serial.println("[DEBUG] Starting I2C Init...");
    // 3. Initialize I2C and OLED Display safely (with timeout so it never hangs)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setTimeOut(25); // 25 ms timeout to prevent any I2C lockup
    delay(50);

    Serial.println("[DEBUG] Scanning for OLED at 0x3C...");
    // Scan if OLED exists at 0x3C or 0x3D
    Wire.beginTransmission(0x3C);
    uint8_t i2c_err = Wire.endTransmission();
    Serial.printf("[DEBUG] I2C Scan result: %d\n", i2c_err);
    if (i2c_err == 0) {
        if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            oled_initialized = true;
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(10, 20);
            display.println("QNX ACTUATOR NODE");
            display.setCursor(10, 35);
            display.println("READY & AWAITING");
            display.display();
            Serial.println("[DEBUG] OLED Initialized Successfully.");
        } else {
            Serial.println("[DEBUG] OLED display.begin() failed.");
            oled_initialized = false;
        }
    } else {
        Serial.println("[DEBUG] OLED not found on I2C bus.");
        oled_initialized = false;
    }

    last_valid_command_time = millis();

    Serial.println("\n========================================================");
    Serial.println("  QNX OUTPUT ACTUATOR NODE 2 (ESP32-C3) LIVE & READY");
    Serial.println("  Send: $CMD,2,100,0,0,0,0,0,0,ALL_NORMAL*4E to test!");
    Serial.println("========================================================\n");
}

// ============================================================================
// ARDUINO MAIN LOOP
// ============================================================================
unsigned long last_periodic_ack = 0;

void loop() {
    unsigned long current_time = millis();

    // 1. Process incoming Serial bytes
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                parse_command_packet(rx_buffer);
                rx_index = 0; // Reset buffer
            }
        } else {
            if (rx_index < RX_BUFFER_SIZE - 1) {
                rx_buffer[rx_index++] = c;
            } else {
                rx_index = 0; // Overflow safety flush
            }
        }
    }

    // 2. Hardware Fail-Safe Watchdog
    if (!failsafe_active && (current_time - last_valid_command_time > FAILSAFE_TIMEOUT_MS)) {
        enter_failsafe_state();
    }

    // 3. Periodic OLED Update (5 Hz / 200 ms)
    if (oled_initialized && (current_time - last_display_update >= 200)) {
        last_display_update = current_time;
        update_oled_display();
    }

    // 4. Periodic Serial Status Heartbeat (1 Hz)
    if (current_time - last_periodic_ack >= 1000) {
        last_periodic_ack = current_time;
        send_ack_packet(last_received_seq);
    }

    // 5. Heartbeat LED Blink (Toggle every 500ms)
    if (current_time - last_heartbeat_toggle >= 500) {
        last_heartbeat_toggle = current_time;
        heartbeat_state = !heartbeat_state;
        digitalWrite(PIN_STATUS_LED, heartbeat_state ? HIGH : LOW);
    }
}
