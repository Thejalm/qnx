#include "../include/safety_engine.h"
#include "../include/qnx_compat.h"
#include "../include/event_logger.h"
#include <string.h>

void safety_engine_init(SafetyEngineContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(SafetyEngineContext));

    ctx->current_state = SYSTEM_STATE_NORMAL;
    // Default safe engineering thresholds
    ctx->thresholds.temp_warning_celsius       = 35.0f;
    ctx->thresholds.temp_critical_celsius      = 50.0f;
    ctx->thresholds.mq2_raw_warning_threshold  = 1000;
    ctx->thresholds.mq2_raw_critical_threshold = 1800;
    ctx->thresholds.flame_adc_threshold        = 1000;
    ctx->thresholds.comm_timeout_ms            = 1000;
}

void safety_engine_set_thresholds(SafetyEngineContext* ctx, const SafetyThresholds* thresholds) {
    if (ctx && thresholds) {
        ctx->thresholds = *thresholds;
    }
}

SystemSafetyState safety_engine_evaluate(SafetyEngineContext* ctx, 
                                        const TelemetryPacket* tel, 
                                        bool is_comm_healthy,
                                        CommandPacket* out_cmd) {
    if (!ctx || !out_cmd) return SYSTEM_STATE_COMM_FAULT;

    uint64_t t_start_ns = get_monotonic_time_ns();
    SystemSafetyState next_state = SYSTEM_STATE_NORMAL;
    memset(out_cmd, 0, sizeof(CommandPacket));
    out_cmd->node_id = NODE_ID_OUTPUT;

    // 1. Communication Link Health Check
    if (!is_comm_healthy || !tel || !tel->is_valid) {
        next_state = SYSTEM_STATE_COMM_FAULT;
        out_cmd->buzzer1_alarm   = true;
        out_cmd->buzzer2_warning = false;
        out_cmd->relay_fan       = true;  // Fail-safe purge
        out_cmd->relay_pump      = false;
        out_cmd->led_yellow      = true;
        out_cmd->led_red         = true;
        strncpy(out_cmd->status_text, "COMM_LINK_DOWN", sizeof(out_cmd->status_text));
        
        if (ctx->current_state != next_state) {
            event_logger_log(LOG_LEVEL_CRITICAL, "SAFETY_ENG", "Communication link failure with Input Node", 0.0f, 0.0f);
        }
    }
    // 2. Hardware Sensor Fault Check
    else if (tel->fault_flags != 0 || tel->temperature < -50.0f) {
        next_state = SYSTEM_STATE_SENSOR_FAULT;
        out_cmd->buzzer1_alarm   = false;
        out_cmd->buzzer2_warning = true;  // Intermittent alert
        out_cmd->relay_fan       = true;  // Maintain airflow
        out_cmd->relay_pump      = false;
        out_cmd->led_yellow      = true;
        out_cmd->led_red         = false;
        strncpy(out_cmd->status_text, "SENSOR_HW_FAULT", sizeof(out_cmd->status_text));

        if (ctx->current_state != next_state) {
            event_logger_log(LOG_LEVEL_FAULT, "SAFETY_ENG", "Sensor hardware fault flag detected", (float)tel->fault_flags, 0.0f);
        }
    }
    // 3. Priority 1 Safety: Fire / Extreme Thermal Runway
    else if (tel->flame_detected || (tel->flame_raw_adc < ctx->thresholds.flame_adc_threshold) ||
             (tel->temperature >= ctx->thresholds.temp_critical_celsius)) {
        next_state = SYSTEM_STATE_FIRE_CRITICAL;
        out_cmd->buzzer1_alarm   = true;  // Primary loud continuous alarm
        out_cmd->buzzer2_warning = false;
        out_cmd->relay_fan       = true;  // Exhaust smoke
        out_cmd->relay_pump      = true;  // Activate water suppression
        out_cmd->led_yellow      = false;
        out_cmd->led_red         = true;  // Red Alert
        strncpy(out_cmd->status_text, "FIRE_CRITICAL", sizeof(out_cmd->status_text));

        if (ctx->current_state != next_state) {
            event_logger_log(LOG_LEVEL_CRITICAL, "SAFETY_ENG", "FIRE ALARM TRIPPED - ACTIVATING SUPPRESSION", tel->temperature, (float)tel->flame_raw_adc);
        }
    }
    // 4. Priority 2 Safety: Gas Leakage Hazard
    else if (tel->mq2_digital_alert || (tel->mq2_raw_adc >= ctx->thresholds.mq2_raw_critical_threshold)) {
        next_state = SYSTEM_STATE_GAS_WARNING;
        out_cmd->buzzer1_alarm   = false;
        out_cmd->buzzer2_warning = true;  // Beep warning
        out_cmd->relay_fan       = true;  // Exhaust fan ON to dilute gas
        out_cmd->relay_pump      = false;
        out_cmd->led_yellow      = true;  // Yellow Warning LED
        out_cmd->led_red         = false;
        strncpy(out_cmd->status_text, "GAS_LEAK_WARN", sizeof(out_cmd->status_text));

        if (ctx->current_state != next_state) {
            event_logger_log(LOG_LEVEL_WARN, "SAFETY_ENG", "Flammable Gas Threshold Exceeded", (float)tel->mq2_raw_adc, 0.0f);
        }
    }
    // 5. Priority 3: Elevated Temperature Warning
    else if (tel->temperature >= ctx->thresholds.temp_warning_celsius) {
        next_state = SYSTEM_STATE_ELEVATED_TEMP;
        out_cmd->buzzer1_alarm   = false;
        out_cmd->buzzer2_warning = false;
        out_cmd->relay_fan       = true;  // Cooling fan ON
        out_cmd->relay_pump      = false;
        out_cmd->led_yellow      = true;
        out_cmd->led_red         = false;
        strncpy(out_cmd->status_text, "ELEVATED_TEMP", sizeof(out_cmd->status_text));
    }
    // 6. Normal Condition
    else {
        next_state = SYSTEM_STATE_NORMAL;
        out_cmd->buzzer1_alarm   = false;
        out_cmd->buzzer2_warning = false;
        out_cmd->relay_fan       = false;
        out_cmd->relay_pump      = false;
        out_cmd->led_yellow      = false;
        out_cmd->led_red         = false;
        strncpy(out_cmd->status_text, "SYSTEM_NORMAL", sizeof(out_cmd->status_text));
    }

    ctx->current_state = next_state;
    ctx->last_eval_latency_ns = get_monotonic_time_ns() - t_start_ns;
    return next_state;
}

const char* safety_state_to_string(SystemSafetyState state) {
    switch (state) {
        case SYSTEM_STATE_NORMAL:        return "NORMAL";
        case SYSTEM_STATE_ELEVATED_TEMP: return "ELEVATED_TEMP";
        case SYSTEM_STATE_GAS_WARNING:   return "GAS_WARNING";
        case SYSTEM_STATE_FIRE_CRITICAL: return "FIRE_CRITICAL";
        case SYSTEM_STATE_SENSOR_FAULT:  return "SENSOR_FAULT";
        case SYSTEM_STATE_COMM_FAULT:    return "COMM_FAULT";
        default:                         return "UNKNOWN";
    }
}
