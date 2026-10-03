#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include "safety_engine.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_FAULT
} EventLogLevel;

typedef struct {
    uint32_t      event_id;
    uint64_t      timestamp_ns;
    EventLogLevel level;
    char          source[16];
    char          description[128];
    float         sensor_val1;
    float         sensor_val2;
    bool          synced_to_backend;
} SafetyEventRecord;

#define MAX_RING_BUFFER_EVENTS 256

void event_logger_init(const char* log_file_path);
void event_logger_log(EventLogLevel level, const char* source, const char* description, float val1, float val2);
size_t event_logger_get_unsynced_events(SafetyEventRecord* out_records, size_t max_count);
void event_logger_mark_synced(uint32_t event_id);
void event_logger_flush(void);

#ifdef __cplusplus
}
#endif

#endif // EVENT_LOGGER_H
