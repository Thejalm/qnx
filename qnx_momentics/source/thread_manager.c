#include "../include/thread_manager.h"
#include "../include/event_logger.h"
#include "../include/qnx_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(IS_NATIVE_QNX) && IS_NATIVE_QNX == 1

static void* safety_decision_thread_func(void* arg) {
    QnxOrchestratorRuntime* rt = (QnxOrchestratorRuntime*)arg;
    struct _msg_info info;
    QnxTelemetryMsg msg;
    QnxReplyHeader reply;

    printf("[QNX CORE] Safety Decision Thread Running (SCHED_FIFO Priority 25)\n");

    while (rt->is_running) {
        // Wait for telemetry message on safety channel
        int rcvid = MsgReceive(rt->ipc.safety_chid, &msg, sizeof(msg), &info);
        if (rcvid < 0) continue;

        // Process message
        if (msg.header.msg_type == QNX_MSG_TYPE_EVAL_SAFETY) {
            CommandPacket cmd;
            bool is_healthy = fault_manager_is_comm_healthy(&rt->fault, rt->safety.thresholds.comm_timeout_ms);
            SystemSafetyState state = safety_engine_evaluate(&rt->safety, &msg.telemetry, is_healthy, &cmd);
            (void)state;
            cmd.sequence_number = msg.telemetry.sequence_number;
            cmd.eval_latency_us = (float)rt->safety.last_eval_latency_ns / 1000.0f;
            cmd.loop_latency_ms = 100.0f;

            // Reply immediately to telemetry sender
            reply.status_code = 0;
            reply.sequence_ack = msg.telemetry.sequence_number;
            MsgReply(rcvid, 0, &reply, sizeof(reply));

            // Forward command to Output Controller Channel via MsgSend
            QnxActuatorCmdMsg cmd_msg;
            cmd_msg.header.msg_type = QNX_MSG_TYPE_ACTUATOR_CMD;
            cmd_msg.header.msg_size = sizeof(cmd_msg);
            cmd_msg.command = cmd;
            MsgSend(rt->ipc.output_coid, &cmd_msg, sizeof(cmd_msg), &reply, sizeof(reply));
        } else {
            reply.status_code = -1;
            MsgReply(rcvid, 0, &reply, sizeof(reply));
        }
    }
    return NULL;
}

static void* output_controller_thread_func(void* arg) {
    QnxOrchestratorRuntime* rt = (QnxOrchestratorRuntime*)arg;
    struct _msg_info info;
    QnxActuatorCmdMsg cmd_msg;
    QnxReplyHeader reply;
    char cmd_buffer[PROTOCOL_MAX_PACKET_LEN];
    char ack_buffer[PROTOCOL_MAX_PACKET_LEN];

    printf("[QNX CORE] Output Controller Thread Running (SCHED_FIFO Priority 22)\n");

    while (rt->is_running) {
        int rcvid = MsgReceive(rt->ipc.output_chid, &cmd_msg, sizeof(cmd_msg), &info);
        if (rcvid < 0) continue;

        if (cmd_msg.header.msg_type == QNX_MSG_TYPE_ACTUATOR_CMD) {
            size_t len = protocol_format_command(&cmd_msg.command, cmd_buffer, sizeof(cmd_buffer));
            if (len > 0) {
                comm_manager_send_command(cmd_buffer, len);
                // Receive ACK with short timeout
                if (comm_manager_receive_ack(ack_buffer, sizeof(ack_buffer), 100)) {
                    AckPacket ack;
                    protocol_parse_ack(ack_buffer, &ack);
                }
            }
            reply.status_code = 0;
            reply.sequence_ack = cmd_msg.command.sequence_number;
            MsgReply(rcvid, 0, &reply, sizeof(reply));
        } else {
            reply.status_code = -1;
            MsgReply(rcvid, 0, &reply, sizeof(reply));
        }
    }
    return NULL;
}

static void* comm_rx_thread_func(void* arg) {
    QnxOrchestratorRuntime* rt = (QnxOrchestratorRuntime*)arg;
    char rx_buf[PROTOCOL_MAX_PACKET_LEN];
    QnxTelemetryMsg msg;
    QnxReplyHeader reply;

    printf("[QNX CORE] Communication Manager Thread Running (SCHED_FIFO Priority 20)\n");

    while (rt->is_running) {
        if (comm_manager_receive_telemetry(rx_buf, sizeof(rx_buf), 500)) {
            TelemetryPacket tel;
            if (protocol_parse_telemetry(rx_buf, &tel)) {
                fault_manager_process_packet(&rt->fault, &tel);

                // Send to Safety Decision Channel
                msg.header.msg_type = QNX_MSG_TYPE_EVAL_SAFETY;
                msg.header.msg_size = sizeof(msg);
                msg.telemetry = tel;
                msg.arrival_timestamp_ns = get_monotonic_time_ns();

                MsgSend(rt->ipc.safety_coid, &msg, sizeof(msg), &reply, sizeof(reply));
            } else {
                fault_manager_record_corruption(&rt->fault);
            }
        } else {
            fault_manager_record_timeout(&rt->fault);
        }
    }
    return NULL;
}

bool thread_manager_start(QnxOrchestratorRuntime* rt) {
    if (!rt) return false;
    rt->is_running = true;

    pthread_t tid_safety, tid_output, tid_rx;
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // Safety Decision Thread: Priority 25
    param.sched_priority = 25;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&tid_safety, &attr, safety_decision_thread_func, rt);

    // Output Controller Thread: Priority 22
    param.sched_priority = 22;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&tid_output, &attr, output_controller_thread_func, rt);

    // Comm RX Thread: Priority 20
    param.sched_priority = 20;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&tid_rx, &attr, comm_rx_thread_func, rt);

    pthread_attr_destroy(&attr);
    return true;
}

void thread_manager_stop(QnxOrchestratorRuntime* rt) {
    if (rt) rt->is_running = false;
}

#else

bool thread_manager_start(QnxOrchestratorRuntime* rt) {
    if (!rt) return false;
    rt->is_running = true;
    return true;
}

void thread_manager_stop(QnxOrchestratorRuntime* rt) {
    if (rt) rt->is_running = false;
}

#endif
