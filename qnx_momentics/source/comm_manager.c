#include "../include/comm_manager.h"
#include "../include/qnx_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
static SOCKET s_input_sock = INVALID_SOCKET;
static SOCKET s_output_sock = INVALID_SOCKET;
static SOCKET s_listen_in = INVALID_SOCKET;
static SOCKET s_listen_out = INVALID_SOCKET;
#else
static int s_input_sock = -1;
static int s_output_sock = -1;
static int s_listen_in = -1;
static int s_listen_out = -1;
#endif

static CommConfig s_config;

// Internal stream buffer for telemetry framing
static char s_rx_stream_buf[2048];
static size_t s_rx_stream_len = 0;

static bool init_server_listeners(int in_port, int out_port) {
    // 1. Input telemetry listener (port 9001)
    s_listen_in = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_in >= 0) {
        int opt = 1;
        setsockopt(s_listen_in, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        
        struct sockaddr_in in_addr;
        memset(&in_addr, 0, sizeof(in_addr));
        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons((uint16_t)in_port);
        in_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(s_listen_in, (struct sockaddr*)&in_addr, sizeof(in_addr)) == 0) {
            listen(s_listen_in, 5);
            printf("[QNX COMM] Telemetry listener active on port %d\n", in_port);
        } else {
            printf("[QNX COMM WARN] Failed to bind telemetry listener port %d: %s\n", in_port, strerror(errno));
        }
    }

    // 2. Output command listener (port 9002)
    s_listen_out = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_out >= 0) {
        int opt = 1;
        setsockopt(s_listen_out, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        struct sockaddr_in out_addr;
        memset(&out_addr, 0, sizeof(out_addr));
        out_addr.sin_family = AF_INET;
        out_addr.sin_port = htons((uint16_t)out_port);
        out_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(s_listen_out, (struct sockaddr*)&out_addr, sizeof(out_addr)) == 0) {
            listen(s_listen_out, 5);
            printf("[QNX COMM] Actuator command listener active on port %d\n", out_port);
        } else {
            printf("[QNX COMM WARN] Failed to bind actuator listener port %d: %s\n", out_port, strerror(errno));
        }
    }
    fflush(stdout);
    return true;
}

bool comm_manager_init(const CommConfig* config) {
    if (!config) return false;
    s_config = *config;
    s_rx_stream_len = 0;
    s_rx_stream_buf[0] = '\0';

#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }
#else
    init_server_listeners(s_config.input_node_port, s_config.output_node_port);
#endif

    s_config.is_running = true;
    return true;
}

bool comm_manager_receive_telemetry(char* out_buffer, size_t max_len, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!out_buffer || max_len == 0) return false;

#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    if (s_input_sock == INVALID_SOCKET) {
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons((u_short)s_config.input_node_port);
        serv_addr.sin_addr.s_addr = inet_addr(s_config.input_node_ip);
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
                s_input_sock = sock;
            } else {
                closesocket(sock);
                return false;
            }
        } else {
            return false;
        }
    }
#else
    // 1. Check for incoming connection on s_listen_in if not connected
    if (s_input_sock < 0 && s_listen_in >= 0) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(s_listen_in, &read_fds);
        struct timeval tv = {0, 10000}; // 10ms
        if (select(s_listen_in + 1, &read_fds, NULL, NULL, &tv) > 0) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(s_listen_in, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock >= 0) {
                s_input_sock = client_sock;
                struct timeval rcv_tv = {0, 100000}; // 100ms
                setsockopt(s_input_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_tv, sizeof(rcv_tv));
                printf("[QNX COMM] Telemetry input node connected from %s:%d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                fflush(stdout);
                s_rx_stream_len = 0;
                s_rx_stream_buf[0] = '\0';
            }
        }
    }

    // 2. Also attempt proactive outbound connection to input_node_ip if not connected
    if (s_input_sock < 0 && s_config.input_node_ip[0] != '\0') {
        static uint64_t s_last_conn_try_ns = 0;
        uint64_t now_ns = get_monotonic_time_ns();
        if (now_ns - s_last_conn_try_ns >= 1000000000ULL) { // Try every 1.0 second
            s_last_conn_try_ns = now_ns;
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                struct sockaddr_in serv_addr;
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons((uint16_t)s_config.input_node_port);
                serv_addr.sin_addr.s_addr = inet_addr(s_config.input_node_ip);
                struct timeval rcv_tv = {0, 200000}; // 200ms
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_tv, sizeof(rcv_tv));
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&rcv_tv, sizeof(rcv_tv));
                if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
                    s_input_sock = sock;
                    printf("[QNX COMM] Successfully connected outbound to Input Node (%s:%d)!\n",
                           s_config.input_node_ip, s_config.input_node_port);
                    fflush(stdout);
                    s_rx_stream_len = 0;
                    s_rx_stream_buf[0] = '\0';
                } else {
                    close(sock);
                }
            }
        }
    }

    if (s_input_sock < 0) {
        return false;
    }
#endif

    // 2. First check if we already have a complete line in stream buffer
    while (s_rx_stream_len > 0) {
        char* newline_pos = strchr(s_rx_stream_buf, '\n');
        if (!newline_pos) break;

        char* start_pos = strchr(s_rx_stream_buf, '$');
        if (start_pos && start_pos < newline_pos) {
            size_t line_len = (size_t)(newline_pos - start_pos) + 1;
            if (line_len < max_len) {
                memcpy(out_buffer, start_pos, line_len);
                out_buffer[line_len] = '\0';
                while (line_len > 0 && (out_buffer[line_len - 1] == '\n' || out_buffer[line_len - 1] == '\r')) {
                    out_buffer[--line_len] = '\0';
                }
                size_t consumed = (size_t)(newline_pos - s_rx_stream_buf) + 1;
                s_rx_stream_len -= consumed;
                memmove(s_rx_stream_buf, newline_pos + 1, s_rx_stream_len);
                s_rx_stream_buf[s_rx_stream_len] = '\0';
                return true;
            }
        }
        // Discard leading garbage up to and including newline
        size_t consumed = (size_t)(newline_pos - s_rx_stream_buf) + 1;
        s_rx_stream_len -= consumed;
        memmove(s_rx_stream_buf, newline_pos + 1, s_rx_stream_len);
        s_rx_stream_buf[s_rx_stream_len] = '\0';
    }

    // 3. Check socket for available incoming data using select()
#if !defined(_WIN32) || defined(__QNX__) || defined(__QNXNTO__)
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(s_input_sock, &read_fds);
    struct timeval tv = {0, 25000}; // 25ms
    int sel_res = select(s_input_sock + 1, &read_fds, NULL, NULL, &tv);
    if (sel_res <= 0) {
        return false; // No data available right now
    }
#endif

    // 4. Read chunk from socket
    char chunk[256];
    int res = (int)recv(s_input_sock, chunk, sizeof(chunk) - 1, 0);
    if (res <= 0) {
        if (res == 0) {
            printf("[QNX COMM] Telemetry stream disconnected by client.\n");
            fflush(stdout);
#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
            closesocket(s_input_sock);
            s_input_sock = INVALID_SOCKET;
#else
            close(s_input_sock);
            s_input_sock = -1;
#endif
            s_rx_stream_len = 0;
            s_rx_stream_buf[0] = '\0';
            return false;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return false;
        }
#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
        closesocket(s_input_sock);
        s_input_sock = INVALID_SOCKET;
#else
        close(s_input_sock);
        s_input_sock = -1;
#endif
        s_rx_stream_len = 0;
        s_rx_stream_buf[0] = '\0';
        return false;
    }

    // Append to stream buffer
    if (s_rx_stream_len + (size_t)res < sizeof(s_rx_stream_buf) - 1) {
        memcpy(s_rx_stream_buf + s_rx_stream_len, chunk, res);
        s_rx_stream_len += (size_t)res;
        s_rx_stream_buf[s_rx_stream_len] = '\0';
    } else {
        s_rx_stream_len = 0;
        s_rx_stream_buf[0] = '\0';
    }

    // Extract line if now complete
    while (s_rx_stream_len > 0) {
        char* newline_pos = strchr(s_rx_stream_buf, '\n');
        if (!newline_pos) break;

        char* start_pos = strchr(s_rx_stream_buf, '$');
        if (start_pos && start_pos < newline_pos) {
            size_t line_len = (size_t)(newline_pos - start_pos) + 1;
            if (line_len < max_len) {
                memcpy(out_buffer, start_pos, line_len);
                out_buffer[line_len] = '\0';
                while (line_len > 0 && (out_buffer[line_len - 1] == '\n' || out_buffer[line_len - 1] == '\r')) {
                    out_buffer[--line_len] = '\0';
                }
                size_t consumed = (size_t)(newline_pos - s_rx_stream_buf) + 1;
                s_rx_stream_len -= consumed;
                memmove(s_rx_stream_buf, newline_pos + 1, s_rx_stream_len);
                s_rx_stream_buf[s_rx_stream_len] = '\0';
                return true;
            }
        }
        // Discard leading garbage up to and including newline
        size_t consumed = (size_t)(newline_pos - s_rx_stream_buf) + 1;
        s_rx_stream_len -= consumed;
        memmove(s_rx_stream_buf, newline_pos + 1, s_rx_stream_len);
        s_rx_stream_buf[s_rx_stream_len] = '\0';
    }

    return false;
}

bool comm_manager_send_command(const char* cmd_buffer, size_t len) {
    if (!cmd_buffer || len == 0) return false;

#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    if (s_output_sock == INVALID_SOCKET) {
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons((u_short)s_config.output_node_port);
        serv_addr.sin_addr.s_addr = inet_addr(s_config.output_node_ip);
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
            if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
                s_output_sock = sock;
            } else {
                closesocket(sock);
                return false;
            }
        } else {
            return false;
        }
    }
    int sent = send(s_output_sock, cmd_buffer, (int)len, 0);
    if (sent <= 0) {
        closesocket(s_output_sock);
        s_output_sock = INVALID_SOCKET;
        return false;
    }
    return true;
#else
    // Check listener for output node connection
    if (s_output_sock < 0 && s_listen_out >= 0) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(s_listen_out, &read_fds);
        struct timeval tv = {0, 1000}; // 1ms
        if (select(s_listen_out + 1, &read_fds, NULL, NULL, &tv) > 0) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(s_listen_out, (struct sockaddr*)&client_addr, &addr_len);
            if (client_sock >= 0) {
                s_output_sock = client_sock;
                struct timeval rcv_tv = {0, 100000};
                setsockopt(s_output_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_tv, sizeof(rcv_tv));
                printf("[QNX COMM] Output actuator node connected from %s:%d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                fflush(stdout);
            }
        }
    }

    // Also attempt outbound connection to output_node_ip if not connected
    if (s_output_sock < 0 && s_config.output_node_ip[0] != '\0') {
        static uint64_t s_last_out_try_ns = 0;
        uint64_t now_ns = get_monotonic_time_ns();
        if (now_ns - s_last_out_try_ns >= 1000000000ULL) {
            s_last_out_try_ns = now_ns;
            int out_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (out_fd >= 0) {
                struct sockaddr_in serv_addr;
                memset(&serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons((uint16_t)s_config.output_node_port);
                serv_addr.sin_addr.s_addr = inet_addr(s_config.output_node_ip);
                struct timeval tv = {0, 200000};
                setsockopt(out_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
                setsockopt(out_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
                if (connect(out_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
                    s_output_sock = out_fd;
                    printf("[QNX COMM] Connected outbound to Output Node (%s:%d)!\n",
                           s_config.output_node_ip, s_config.output_node_port);
                    fflush(stdout);
                } else {
                    close(out_fd);
                }
            }
        }
    }

    if (s_output_sock < 0) return false;

    ssize_t sent = send(s_output_sock, cmd_buffer, len, 0);
    if (sent <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close(s_output_sock);
            s_output_sock = -1;
        }
        return false;
    }
    return true;
#endif
}

bool comm_manager_receive_ack(char* out_buffer, size_t max_len, uint32_t timeout_ms) {
    if (!out_buffer || max_len == 0) return false;

#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    if (s_output_sock == INVALID_SOCKET) return false;
#else
    if (s_output_sock < 0) return false;

    // Fast non-blocking check with timeout_ms to protect deterministic deadline
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(s_output_sock, &read_fds);
    struct timeval tv = {0, (long)(timeout_ms * 1000)};
    int sel_res = select(s_output_sock + 1, &read_fds, NULL, NULL, &tv);
    if (sel_res <= 0) {
        return false; // No ACK available right now
    }
#endif

    size_t idx = 0;
    char c = 0;
    bool found_start = false;

    while (idx < max_len - 1) {
        int res = (int)recv(s_output_sock, &c, 1, 0);
        if (res <= 0) {
            break;
        }
        if (c == '$') {
            found_start = true;
            idx = 0;
            out_buffer[idx++] = c;
        } else if (found_start) {
            if (c == '\n' || c == '\r') {
                if (idx > 3) {
                    out_buffer[idx] = '\0';
                    return true;
                }
            } else {
                out_buffer[idx++] = c;
            }
        }
    }

    if (idx > 0) {
        out_buffer[idx] = '\0';
        return true;
    }
    return false;
}

void comm_manager_shutdown(void) {
#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    if (s_input_sock != INVALID_SOCKET) { closesocket(s_input_sock); s_input_sock = INVALID_SOCKET; }
    if (s_output_sock != INVALID_SOCKET) { closesocket(s_output_sock); s_output_sock = INVALID_SOCKET; }
    if (s_listen_in != INVALID_SOCKET) { closesocket(s_listen_in); s_listen_in = INVALID_SOCKET; }
    if (s_listen_out != INVALID_SOCKET) { closesocket(s_listen_out); s_listen_out = INVALID_SOCKET; }
    WSACleanup();
#else
    if (s_input_sock >= 0) { close(s_input_sock); s_input_sock = -1; }
    if (s_output_sock >= 0) { close(s_output_sock); s_output_sock = -1; }
    if (s_listen_in >= 0) { close(s_listen_in); s_listen_in = -1; }
    if (s_listen_out >= 0) { close(s_listen_out); s_listen_out = -1; }
#endif
}
