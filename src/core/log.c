#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

// Note: Single-threaded design; add mutex if multi-threaded later
static int current_level = WINAFI_LOG_INFO;

void log_init(int level) {
    current_level = level;
}

void log_msg(int level, const char *fmt, ...) {
    // Validate format string to prevent NULL pointer dereference
    if (!fmt) fmt = "(null format)";

    if (level < current_level) return;

    const char *level_str = "INFO";
    FILE *out = stdout;

    switch (level) {
        case WINAFI_LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case WINAFI_LOG_ERROR:
            level_str = "ERROR";
            out = stderr;
            break;
        default:
            level_str = "INFO";
    }

    // Get current time and safely format timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    if (tm_info) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(timestamp, sizeof(timestamp), "UNKNOWN_TIME");
    }

    fprintf(out, "[%s] [%s] ", timestamp, level_str);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
    fflush(out);
}

// Public API wrapper
void winafi_set_log_level(int level) {
    log_init(level);
}
