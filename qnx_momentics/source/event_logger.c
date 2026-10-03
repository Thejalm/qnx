#include "../include/event_logger.h"
#include "../include/qnx_compat.h"
#include <stdio.h>
#include <string.h>

static SafetyEventRecord s_ring_buffer[MAX_RING_BUFFER_EVENTS];
static size_t s_head_idx = 0;
static size_t s_total_events = 0;
static uint32_t s_next_event_id = 1;
static char s_log_file_path[256] = "qnx_safety_events.log";

#if !defined(_WIN32)
static pthread_mutex_t s_log_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
static CRITICAL_SECTION s_log_cs;
static bool s_cs_init = false;
#endif

void event_logger_init(const char* log_file_path) {
    if (log_file_path && strlen(log_file_path) > 0) {
        strncpy(s_log_file_path, log_file_path, sizeof(s_log_file_path) - 1);
    }
#if defined(_WIN32)
    if (!s_cs_init) {
        InitializeCriticalSection(&s_log_cs);
        s_cs_init = true;
    }
#endif
    memset(s_ring_buffer, 0, sizeof(s_ring_buffer));
    s_head_idx = 0;
    s_total_events = 0;
    s_next_event_id = 1;
}

static void lock_logger(void) {
#if defined(_WIN32)
    EnterCriticalSection(&s_log_cs);
#else
    pthread_mutex_lock(&s_log_mutex);
#endif
}

static void unlock_logger(void) {
#if defined(_WIN32)
    LeaveCriticalSection(&s_log_cs);
#else
    pthread_mutex_unlock(&s_log_mutex);
#endif
}

void event_logger_log(EventLogLevel level, const char* source, const char* description, float val1, float val2) {
    lock_logger();

    SafetyEventRecord* rec = &s_ring_buffer[s_head_idx];
    rec->event_id = s_next_event_id++;
    rec->timestamp_ns = get_monotonic_time_ns();
    rec->level = level;
    strncpy(rec->source, source ? source : "QNX_CORE", sizeof(rec->source) - 1);
    strncpy(rec->description, description ? description : "Safety Event", sizeof(rec->description) - 1);
    rec->sensor_val1 = val1;
    rec->sensor_val2 = val2;
    rec->synced_to_backend = false;

    s_head_idx = (s_head_idx + 1) % MAX_RING_BUFFER_EVENTS;
    s_total_events++;

    // Immediate append to local disk file (fault-tolerant against sudden power loss)
    FILE* f = fopen(s_log_file_path, "a");
    if (f) {
        const char* lvl_str = (level == LOG_LEVEL_CRITICAL) ? "CRITICAL" :
                              (level == LOG_LEVEL_WARN) ? "WARNING" :
                              (level == LOG_LEVEL_FAULT) ? "FAULT" : "INFO";
        fprintf(f, "[%llu] [%s] [%s] %s | V1=%.2f V2=%.2f\n",
                (unsigned long long)rec->timestamp_ns, lvl_str, rec->source, rec->description, val1, val2);
        fclose(f);
    }

    unlock_logger();
}

size_t event_logger_get_unsynced_events(SafetyEventRecord* out_records, size_t max_count) {
    if (!out_records || max_count == 0) return 0;
    lock_logger();

    size_t count = 0;
    size_t inspected = 0;
    size_t idx = (s_head_idx + MAX_RING_BUFFER_EVENTS - 1) % MAX_RING_BUFFER_EVENTS;

    while (inspected < MAX_RING_BUFFER_EVENTS && count < max_count) {
        if (s_ring_buffer[idx].event_id > 0 && !s_ring_buffer[idx].synced_to_backend) {
            out_records[count++] = s_ring_buffer[idx];
        }
        idx = (idx + MAX_RING_BUFFER_EVENTS - 1) % MAX_RING_BUFFER_EVENTS;
        inspected++;
    }

    unlock_logger();
    return count;
}

void event_logger_mark_synced(uint32_t event_id) {
    lock_logger();
    for (size_t i = 0; i < MAX_RING_BUFFER_EVENTS; i++) {
        if (s_ring_buffer[i].event_id == event_id) {
            s_ring_buffer[i].synced_to_backend = true;
            break;
        }
    }
    unlock_logger();
}

void event_logger_flush(void) {
    // Disk writes are unbuffered and synced per event
}
