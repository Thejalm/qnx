#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include "protocol.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_packets_received;
    uint32_t total_corrupt_packets;
    uint32_t total_dropped_packets;
    uint32_t consecutive_timeouts;
    uint32_t last_received_seq;
    uint64_t last_valid_packet_time_ns;
    uint32_t active_fault_bitmask;
} FaultManagerContext;

void fault_manager_init(FaultManagerContext* ctx);
bool fault_manager_process_packet(FaultManagerContext* ctx, const TelemetryPacket* telemetry);
void fault_manager_record_timeout(FaultManagerContext* ctx);
void fault_manager_record_corruption(FaultManagerContext* ctx);
bool fault_manager_is_comm_healthy(const FaultManagerContext* ctx, uint32_t timeout_threshold_ms);

#ifdef __cplusplus
}
#endif

#endif // FAULT_MANAGER_H
