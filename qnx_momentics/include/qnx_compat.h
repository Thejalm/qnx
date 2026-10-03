#ifndef QNX_COMPAT_H
#define QNX_COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <sys/neutrino.h>
    #include <sys/netmgr.h>
    #include <sys/dispatch.h>
    #include <sys/procmgr.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
    #define IS_NATIVE_QNX 1
#else
    #define IS_NATIVE_QNX 0
    #if defined(_WIN32)
        #include <winsock2.h>
        #include <ws2tcpip.h>
        #include <windows.h>
        #include <process.h>
    #else
        #include <pthread.h>
        #include <sched.h>
        #include <unistd.h>
        #include <sys/socket.h>
        #include <sys/time.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
    #endif
#endif

// QNX-specific Message Passing pulse codes
#define QNX_PULSE_CODE_TELEMETRY   0x01
#define QNX_PULSE_CODE_TIMEOUT     0x02
#define QNX_PULSE_CODE_SAFETY_TRIP 0x03

// Message passing structure
typedef struct {
    uint16_t msg_type;
    uint16_t payload_size;
    char     payload_data[256];
} QnxIpcMessage;

// Timing helper returning monotonic nanoseconds
static inline uint64_t get_monotonic_time_ns(void) {
#if defined(_WIN32) && !defined(__QNX__) && !defined(__QNXNTO__)
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
#endif
}

#endif // QNX_COMPAT_H
