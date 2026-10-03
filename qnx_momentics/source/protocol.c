#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t protocol_compute_xor_checksum(const char* data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= (uint8_t)data[i];
    }
    return checksum;
}

bool protocol_parse_telemetry(const char* raw_frame, TelemetryPacket* out) {
    if (!raw_frame || !out) return false;
    memset(out, 0, sizeof(TelemetryPacket));

    // Must start with '$' and contain '*'
    if (raw_frame[0] != PROTOCOL_START_DELIM) return false;
    const char* star = strchr(raw_frame, PROTOCOL_CHECKSUM_DELIM);
    if (!star) return false;

    // Extract Checksum
    uint8_t expected_chk = (uint8_t)strtol(star + 1, NULL, 16);
    size_t payload_len = (size_t)(star - (raw_frame + 1));
    
    char payload[PROTOCOL_MAX_PACKET_LEN];
    if (payload_len >= sizeof(payload)) return false;
    memcpy(payload, raw_frame + 1, payload_len);
    payload[payload_len] = '\0';

    uint8_t actual_chk = protocol_compute_xor_checksum(payload, payload_len);
    if (expected_chk != actual_chk) {
        return false; // Checksum failure
    }

    // Tokenize CSV: IN,node_id,seq,ts,temp,hum,press,mq2_adc,mq2_alert,flame_det,flame_adc,fault_flags
    char* saveptr = NULL;
    char* token = strtok_r(payload, ",", &saveptr);
    if (!token || strcmp(token, "IN") != 0) return false;

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->node_id = (uint8_t)atoi(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->sequence_number = (uint32_t)strtoul(token, NULL, 10);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->timestamp_ms = (uint32_t)strtoul(token, NULL, 10);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->temperature = (float)atof(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->humidity = (float)atof(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->pressure = (float)atof(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->mq2_raw_adc = (int16_t)atoi(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->mq2_digital_alert = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->flame_detected = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->flame_raw_adc = (int16_t)atoi(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->fault_flags = (uint8_t)atoi(token);

    out->checksum = actual_chk;
    out->is_valid = true;
    return true;
}

bool protocol_parse_ack(const char* raw_frame, AckPacket* out) {
    if (!raw_frame || !out) return false;
    memset(out, 0, sizeof(AckPacket));

    if (raw_frame[0] != PROTOCOL_START_DELIM) return false;
    const char* star = strchr(raw_frame, PROTOCOL_CHECKSUM_DELIM);
    if (!star) return false;

    uint8_t expected_chk = (uint8_t)strtol(star + 1, NULL, 16);
    size_t payload_len = (size_t)(star - (raw_frame + 1));
    char payload[PROTOCOL_MAX_PACKET_LEN];
    if (payload_len >= sizeof(payload)) return false;
    memcpy(payload, raw_frame + 1, payload_len);
    payload[payload_len] = '\0';

    if (expected_chk != protocol_compute_xor_checksum(payload, payload_len)) {
        return false;
    }

    char* saveptr = NULL;
    char* token = strtok_r(payload, ",", &saveptr);
    if (!token || strcmp(token, "ACK") != 0) return false;

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->node_id = (uint8_t)atoi(token);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->sequence_number = (uint32_t)strtoul(token, NULL, 10);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->buzzer1_alarm = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->buzzer2_warning = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->relay_fan = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->relay_pump = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->led_yellow = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->led_red = (atoi(token) != 0);

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) return false;
    out->failsafe_active = (atoi(token) != 0);

    out->is_valid = true;
    return true;
}

size_t protocol_format_command(const CommandPacket* cmd, char* out_buffer, size_t max_len) {
    if (!cmd || !out_buffer || max_len < 64) return 0;

    char payload[PROTOCOL_MAX_PACKET_LEN];
    snprintf(payload, sizeof(payload),
             "CMD,%u,%lu,%d,%d,%d,%d,%d,%d,%s,%.1f,%.1f",
             cmd->node_id,
             (unsigned long)cmd->sequence_number,
             cmd->buzzer1_alarm ? 1 : 0,
             cmd->buzzer2_warning ? 1 : 0,
             cmd->relay_fan ? 1 : 0,
             cmd->relay_pump ? 1 : 0,
             cmd->led_yellow ? 1 : 0,
             cmd->led_red ? 1 : 0,
             cmd->status_text,
             cmd->eval_latency_us,
             cmd->loop_latency_ms);

    uint8_t chk = protocol_compute_xor_checksum(payload, strlen(payload));
    int written = snprintf(out_buffer, max_len, "$%s*%02X\r\n", payload, chk);
    return (written > 0 && (size_t)written < max_len) ? (size_t)written : 0;
}
