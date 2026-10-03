#ifndef QNX_IPC_H
#define QNX_IPC_H

#include "protocol.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// QNX Channel Message Types
#define QNX_MSG_TYPE_TELEMETRY_RAW    0x0101
#define QNX_MSG_TYPE_EVAL_SAFETY      0x0102
#define QNX_MSG_TYPE_ACTUATOR_CMD     0x0103
#define QNX_MSG_TYPE_LOG_EVENT        0x0104
#define QNX_MSG_TYPE_GET_STATUS       0x0105

// QNX Real-Time Pulse Codes (Deterministic Non-Blocking Pulses)
#define QNX_PULSE_HEARTBEAT_TICK      0x10
#define QNX_PULSE_COMM_TIMEOUT        0x11
#define QNX_PULSE_CRITICAL_FIRE       0x12
#define QNX_PULSE_GAS_ALERT           0x13

// QNX IPC Request Header
typedef struct {
    uint16_t msg_type;
    uint16_t msg_size;
} QnxMsgHeader;

// QNX IPC Telemetry Request
typedef struct {
    QnxMsgHeader    header;
    TelemetryPacket telemetry;
    uint64_t        arrival_timestamp_ns;
} QnxTelemetryMsg;

// QNX IPC Actuator Command Request
typedef struct {
    QnxMsgHeader  header;
    CommandPacket command;
} QnxActuatorCmdMsg;

// QNX IPC Reply Header
typedef struct {
    int32_t  status_code;   // 0 = SUCCESS, <0 = ERROR
    uint32_t sequence_ack;
} QnxReplyHeader;

// Channel handles structure
typedef struct {
    int safety_chid;   // Channel ID for Safety Decision Engine
    int safety_coid;   // Connection ID to Safety Decision Channel
    int output_chid;   // Channel ID for Output Actuator Controller
    int output_coid;   // Connection ID to Output Actuator Channel
    int logger_chid;   // Channel ID for Event Logger
    int logger_coid;   // Connection ID to Event Logger Channel
} QnxIpcContext;

bool qnx_ipc_init(QnxIpcContext* ctx);
void qnx_ipc_destroy(QnxIpcContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // QNX_IPC_H
