#include "../include/fault_manager.h"
#include "../include/qnx_compat.h"
#include "../include/event_logger.h"
#include <string.h>

void fault_manager_init(FaultManagerContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(FaultManagerContext));
    ctx->last_valid_packet_time_ns = get_monotonic_time_ns();
}

bool fault_manager_process_packet(FaultManagerContext* ctx, const TelemetryPacket* tel) {
    if (!ctx || !tel) return false;

    ctx->total_packets_received++;

    // Check Sequence continuity (detect dropped packets)
    if (ctx->total_packets_received > 1) {
        uint32_t expected_seq = ctx->last_received_seq + 1;
        if (tel->sequence_number > expected_seq) {
            uint32_t dropped = tel->sequence_number - expected_seq;
            ctx->total_dropped_packets += dropped;
            ctx->active_fault_bitmask |= FAULT_SEQUENCE_DROP;
            event_logger_log(LOG_LEVEL_WARN, "FAULT_MGR", "Packet loss detected on wired link", (float)dropped, (float)tel->sequence_number);
        } else if (tel->sequence_number == expected_seq) {
            ctx->active_fault_bitmask &= ~FAULT_SEQUENCE_DROP;
        }
    }

    ctx->last_received_seq = tel->sequence_number;
    ctx->last_valid_packet_time_ns = get_monotonic_time_ns();
    ctx->consecutive_timeouts = 0;

    // Check embedded hardware fault flags
    if (tel->fault_flags & 0x01) {
        ctx->active_fault_bitmask |= FAULT_BME_DISCONNECT;
    } else {
        ctx->active_fault_bitmask &= ~FAULT_BME_DISCONNECT;
    }

    if (tel->fault_flags & 0x02) {
        ctx->active_fault_bitmask |= FAULT_MQ2_OPEN_SHORT;
    } else {
        ctx->active_fault_bitmask &= ~FAULT_MQ2_OPEN_SHORT;
    }

    if (tel->fault_flags & 0x04) {
        ctx->active_fault_bitmask |= FAULT_FLAME_DEFECT;
    } else {
        ctx->active_fault_bitmask &= ~FAULT_FLAME_DEFECT;
    }

    return true;
}

void fault_manager_record_timeout(FaultManagerContext* ctx) {
    if (!ctx) return;
    ctx->consecutive_timeouts++;
    ctx->active_fault_bitmask |= FAULT_COMM_TIMEOUT;
}

void fault_manager_record_corruption(FaultManagerContext* ctx) {
    if (!ctx) return;
    ctx->total_corrupt_packets++;
    ctx->active_fault_bitmask |= FAULT_CORRUPT_PACKET;
}

bool fault_manager_is_comm_healthy(const FaultManagerContext* ctx, uint32_t timeout_threshold_ms) {
    if (!ctx) return false;
    uint64_t now_ns = get_monotonic_time_ns();
    uint64_t elapsed_ms = (now_ns - ctx->last_valid_packet_time_ns) / 1000000ULL;
    return (elapsed_ms <= (uint64_t)timeout_threshold_ms);
}
