#include "../include/protocol.h"
#include "../include/safety_engine.h"
#include "../include/fault_manager.h"
#include "../include/comm_manager.h"
#include "../include/event_logger.h"
#include "../include/qnx_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile bool g_running = true;

static void sig_handler(int sig) {
    (void)sig;
    g_running = false;
}

int main(int argc, char** argv) {
    printf("====================================================================\n");
    printf("  QNX-BASED REAL-TIME SAFETY & AUTOMATION ORCHESTRATOR (MASTER)    \n");
    printf("====================================================================\n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    // 1. Initialize Configuration
    CommConfig comm_cfg;
    memset(&comm_cfg, 0, sizeof(comm_cfg));
    strncpy(comm_cfg.input_node_ip, "10.61.30.52", sizeof(comm_cfg.input_node_ip) - 1);
    comm_cfg.input_node_port = 9001;
    strncpy(comm_cfg.output_node_ip, "10.61.30.60", sizeof(comm_cfg.output_node_ip) - 1);
    comm_cfg.output_node_port = 9002;
    comm_cfg.backend_sync_port = 8000;

    // Parse command line arguments if provided
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--in-ip") == 0 && i + 1 < argc) {
            strncpy(comm_cfg.input_node_ip, argv[++i], sizeof(comm_cfg.input_node_ip) - 1);
        } else if (strcmp(argv[i], "--out-ip") == 0 && i + 1 < argc) {
            strncpy(comm_cfg.output_node_ip, argv[++i], sizeof(comm_cfg.output_node_ip) - 1);
        } else if (strcmp(argv[i], "--in-port") == 0 && i + 1 < argc) {
            comm_cfg.input_node_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--out-port") == 0 && i + 1 < argc) {
            comm_cfg.output_node_port = atoi(argv[++i]);
        }
    }

    // 2. Initialize Subsystems
    event_logger_init("qnx_safety_events.log");
    event_logger_log(LOG_LEVEL_INFO, "QNX_CORE", "Safety Orchestrator Engine Initialized", 0.0f, 0.0f);

    SafetyEngineContext safety_ctx;
    safety_engine_init(&safety_ctx);

    FaultManagerContext fault_ctx;
    fault_manager_init(&fault_ctx);

    if (!comm_manager_init(&comm_cfg)) {
        fprintf(stderr, "[ERROR] Failed to initialize Communication Manager!\n");
        return 1;
    }

    printf("[QNX CORE] Real-Time Safety Engine Started (SCHED_FIFO Priority Mode)\n");
    printf("[QNX CORE] Listening for Input Telemetry on Port %d & Output Commands on Port %d\n",
           comm_cfg.input_node_port, comm_cfg.output_node_port);
    printf("--------------------------------------------------------------------\n");
    fflush(stdout);

    char rx_buffer[PROTOCOL_MAX_PACKET_LEN];
    char cmd_buffer[PROTOCOL_MAX_PACKET_LEN];
    char ack_buffer[PROTOCOL_MAX_PACKET_LEN];
    uint32_t cmd_seq = 1;

    TelemetryPacket last_valid_tel;
    memset(&last_valid_tel, 0, sizeof(last_valid_tel));
    uint64_t last_packet_time_ns = get_monotonic_time_ns();
    uint64_t last_waiting_print_ns = 0;
    double last_measured_loop_latency_ms = 0.0;

    while (g_running) {
        uint64_t loop_start_ns = get_monotonic_time_ns();

        // Step A: Receive Telemetry from Input Node
        bool got_packet = comm_manager_receive_telemetry(rx_buffer, sizeof(rx_buffer), 100);

        TelemetryPacket tel;
        memset(&tel, 0, sizeof(tel));
        bool is_healthy = true;
        bool should_evaluate = false;

        if (got_packet) {
            bool valid = protocol_parse_telemetry(rx_buffer, &tel);
            if (valid) {
                last_valid_tel = tel;
                last_packet_time_ns = get_monotonic_time_ns();
                fault_manager_process_packet(&fault_ctx, &tel);
                is_healthy = fault_manager_is_comm_healthy(&fault_ctx, safety_ctx.thresholds.comm_timeout_ms);
                should_evaluate = true;
            } else {
                fault_manager_record_corruption(&fault_ctx);
            }
        } else {
            if (last_valid_tel.is_valid) {
                uint64_t elapsed_ms = (get_monotonic_time_ns() - last_packet_time_ns) / 1000000;
                if (elapsed_ms >= (uint64_t)safety_ctx.thresholds.comm_timeout_ms) {
                    fault_manager_record_timeout(&fault_ctx);
                    is_healthy = false;
                    tel = last_valid_tel;
                    should_evaluate = true;
                }
            } else {
                // Not yet received initial packet
                uint64_t now = get_monotonic_time_ns();
                if (now - last_waiting_print_ns >= 2000000000ULL) {
                    last_waiting_print_ns = now;
                    printf("[QNX CORE] Waiting for telemetry stream from Input Node on port %d...\n",
                           comm_cfg.input_node_port);
                    fflush(stdout);
                }
            }
        }

        if (should_evaluate) {
            // Step B: Real-Time Safety Decision Evaluation
            CommandPacket cmd;
            SystemSafetyState state = safety_engine_evaluate(&safety_ctx, &tel, is_healthy, &cmd);
            cmd.sequence_number = cmd_seq++;
            cmd.eval_latency_us = (float)safety_ctx.last_eval_latency_ns / 1000.0f;
            cmd.loop_latency_ms = (float)last_measured_loop_latency_ms;

            // Step C: Transmit Command to Output Node Actuators
            size_t cmd_len = protocol_format_command(&cmd, cmd_buffer, sizeof(cmd_buffer));
            if (cmd_len > 0) {
                comm_manager_send_command(cmd_buffer, cmd_len);
            }

            // Step D: Receive Acknowledgment (ACK)
            AckPacket ack;
            if (comm_manager_receive_ack(ack_buffer, sizeof(ack_buffer), 50)) {
                protocol_parse_ack(ack_buffer, &ack);
            }

            // Step E: Compute Execution & Latency Metrics
            uint64_t loop_end_ns = get_monotonic_time_ns();
            double loop_latency_ms = (double)(loop_end_ns - loop_start_ns) / 1000000.0;
            last_measured_loop_latency_ms = loop_latency_ms;
            double qnx_decision_us = (double)safety_ctx.last_eval_latency_ns / 1000.0;

            // Step F: Real-Time Orchestrator Dashboard Output
            printf("[QNX] State:%-14s | T:%4.1fC Gas:%4d Flame:%d | Fan:%d Pump:%d Buz:%d | Eval:%5.1fus Loop:%5.1fms Dropped:%u\n",
                   safety_state_to_string(state),
                   tel.temperature,
                   tel.mq2_raw_adc,
                   tel.flame_detected ? 1 : 0,
                   cmd.relay_fan ? 1 : 0,
                   cmd.relay_pump ? 1 : 0,
                   cmd.buzzer1_alarm ? 1 : 0,
                   qnx_decision_us,
                   loop_latency_ms,
                   fault_ctx.total_dropped_packets);
            fflush(stdout);
        }

#if defined(_WIN32)
        Sleep(5); // 5ms loop polling
#else
        usleep(5000);
#endif
    }

    printf("\n[QNX CORE] Gracefully shutting down Safety Orchestrator...\n");
    event_logger_log(LOG_LEVEL_INFO, "QNX_CORE", "Safety Orchestrator Shutdown", 0.0f, 0.0f);
    comm_manager_shutdown();
    printf("[QNX CORE] Shutdown complete.\n");
    return 0;
}
