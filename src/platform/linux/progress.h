#ifndef PLATFORM_LINUX_PROGRESS_H
#define PLATFORM_LINUX_PROGRESS_H

#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <linux/limits.h>

// Progress stages
#define PROGRESS_STAGE_VALIDATION  0  // 0-5%
#define PROGRESS_STAGE_PARTITION   1  // 5-10%
#define PROGRESS_STAGE_FORMAT      2  // 10-20%
#define PROGRESS_STAGE_EXTRACT     3  // 20-95%
#define PROGRESS_STAGE_BOOT        4  // 95-100%

// Error codes
#define PROGRESS_OK              0
#define PROGRESS_ERR_NULL        -1
#define PROGRESS_ERR_INVALID     -2

typedef struct {
    // Stage information
    int current_stage;
    int stage_start_percent;
    int stage_end_percent;

    // Byte tracking
    uint64_t total_bytes;
    uint64_t bytes_written;

    // Timing
    time_t stage_start_time;
    time_t operation_start_time;

    // Speed calculation (moving average)
    double speed_mbps;
    uint64_t speed_window_bytes;
    time_t speed_window_start;

    // Current file info
    char current_file[PATH_MAX];
    int current_file_percent;
} progress_context_t;

// Functions
progress_context_t* progress_create(uint64_t total_iso_bytes);
void progress_free(progress_context_t *ctx);
int progress_advance_stage(progress_context_t *ctx, int new_stage);
int progress_get_percent(progress_context_t *ctx);
double progress_get_speed_mbps(progress_context_t *ctx);
int progress_get_time_remaining_seconds(progress_context_t *ctx, int *out_seconds);
int progress_update_extract(progress_context_t *ctx, uint64_t bytes_written,
                           const char *current_file, int file_percent);
int progress_format_message(progress_context_t *ctx, char *out_msg, size_t msg_size,
                           const char *operation_name);

#endif
