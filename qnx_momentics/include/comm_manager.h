#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include "protocol.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char input_node_ip[64];
    int  input_node_port;
    char output_node_ip[64];
    int  output_node_port;
    int  backend_sync_port;
    bool is_running;
} CommConfig;

// Initializes network sockets / channels
bool comm_manager_init(const CommConfig* config);
bool comm_manager_receive_telemetry(char* out_buffer, size_t max_len, uint32_t timeout_ms);
bool comm_manager_send_command(const char* cmd_buffer, size_t len);
bool comm_manager_receive_ack(char* out_buffer, size_t max_len, uint32_t timeout_ms);
void comm_manager_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // COMM_MANAGER_H
