#ifndef WINAFI_LOG_H
#define WINAFI_LOG_H

// Use int for internal function signatures to avoid circular deps
#define WINAFI_LOG_DEBUG 0
#define WINAFI_LOG_INFO 1
#define WINAFI_LOG_ERROR 2

// Initialize logging
void log_init(int level);

// Log a message
void log_msg(int level, const char *fmt, ...);

#define log_debug(fmt, ...) log_msg(WINAFI_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) log_msg(WINAFI_LOG_INFO, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_msg(WINAFI_LOG_ERROR, fmt, ##__VA_ARGS__)

#endif
