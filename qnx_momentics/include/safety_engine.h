#ifndef SAFETY_ENGINE_H
#define SAFETY_ENGINE_H

#include "protocol.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safety Operational States
typedef enum {
    SYSTEM_STATE_NORMAL = 0,
    SYSTEM_STATE_ELEVATED_TEMP,
    SYSTEM_STATE_GAS_WARNING,
    SYSTEM_STATE_FIRE_CRITICAL,
    SYSTEM_STATE_SENSOR_FAULT,
    SYSTEM_STATE_COMM_FAULT
} SystemSafetyState;

// Configurable Safety Thresholds
typedef struct {
    float   temp_warning_celsius;       // Default: 35.0 C
    float   temp_critical_celsius;      // Default: 50.0 C
    int16_t mq2_raw_warning_threshold;  // Default: 1200 ADC
    int16_t mq2_raw_critical_threshold; // Default: 2000 ADC
    int16_t flame_adc_threshold;        // Default: 1000 ADC (lower = more IR)
    uint32_t comm_timeout_ms;           // Default: 1000 ms
} SafetyThresholds;

// Engine context
typedef struct {
    SystemSafetyState current_state;
    SafetyThresholds  thresholds;
    uint32_t          state_entry_time_ms;
    uint32_t          consecutive_critical_evals;
    uint64_t          last_eval_latency_ns;
} SafetyEngineContext;

void safety_engine_init(SafetyEngineContext* ctx);
void safety_engine_set_thresholds(SafetyEngineContext* ctx, const SafetyThresholds* thresholds);
SystemSafetyState safety_engine_evaluate(SafetyEngineContext* ctx, 
                                        const TelemetryPacket* telemetry, 
                                        bool is_comm_healthy,
                                        CommandPacket* out_command);

const char* safety_state_to_string(SystemSafetyState state);

#ifdef __cplusplus
}
#endif

#endif // SAFETY_ENGINE_H
