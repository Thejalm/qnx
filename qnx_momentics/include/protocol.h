#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROTOCOL_START_DELIM   '$'
#define PROTOCOL_CHECKSUM_DELIM '*'
#define PROTOCOL_MAX_PACKET_LEN 256

#define NODE_ID_INPUT          1
#define NODE_ID_OUTPUT         2

// Fault flag bitmasks
#define FAULT_BME_DISCONNECT   (1 << 0)
#define FAULT_MQ2_OPEN_SHORT   (1 << 1)
#define FAULT_FLAME_DEFECT     (1 << 2)
#define FAULT_COMM_TIMEOUT     (1 << 3)
#define FAULT_CORRUPT_PACKET   (1 << 4)
#define FAULT_SEQUENCE_DROP    (1 << 5)

// Parsed Telemetry structure from Input Node
typedef struct {
    uint8_t  node_id;
    uint32_t sequence_number;
    uint32_t timestamp_ms;
    float    temperature;      // deg C
    float    humidity;         // %
    float    pressure;         // hPa
    int16_t  mq2_raw_adc;      // 0 - 4095
    bool     mq2_digital_alert;// 1 = alert
    bool     flame_detected;   // 1 = flame
    int16_t  flame_raw_adc;    // 0 - 4095
    uint8_t  fault_flags;      // Bitmask
    uint8_t  checksum;
    bool     is_valid;
} TelemetryPacket;

// Actuator Command structure to Output Node
typedef struct {
    uint8_t  node_id;
    uint32_t sequence_number;
    bool     buzzer1_alarm;    // Primary Loud Alarm
    bool     buzzer2_warning;  // Warning Beep
    bool     relay_fan;        // Cooling/Exhaust Fan
    bool     relay_pump;       // Water Pump Suppressor
    bool     led_yellow;       // Warning LED
    bool     led_red;          // Critical LED
    char     status_text[32];  // OLED status message
    float    eval_latency_us;  // QNX decision latency in microseconds
    float    loop_latency_ms;  // End-to-end loop latency in milliseconds
} CommandPacket;

// Acknowledgment packet received from Output Node
typedef struct {
    uint8_t  node_id;
    uint32_t sequence_number;
    bool     buzzer1_alarm;
    bool     buzzer2_warning;
    bool     relay_fan;
    bool     relay_pump;
    bool     led_yellow;
    bool     led_red;
    bool     failsafe_active;
    bool     is_valid;
} AckPacket;

// Function prototypes
uint8_t protocol_compute_xor_checksum(const char* data, size_t len);
bool protocol_parse_telemetry(const char* raw_frame, TelemetryPacket* out_telemetry);
bool protocol_parse_ack(const char* raw_frame, AckPacket* out_ack);
size_t protocol_format_command(const CommandPacket* cmd, char* out_buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_H
