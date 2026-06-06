#include "progress.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const int stage_ranges[][2] = {
    {0, 5},    // VALIDATION
    {5, 10},   // PARTITION
    {10, 20},  // FORMAT
    {20, 95},  // EXTRACT
    {95, 100}  // BOOT
};

progress_context_t* progress_create(uint64_t total_iso_bytes) {
    progress_context_t *ctx = malloc(sizeof(progress_context_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(progress_context_t));
    ctx->total_bytes = total_iso_bytes;
    ctx->operation_start_time = time(NULL);
    ctx->stage_start_time = ctx->operation_start_time;
    ctx->current_stage = PROGRESS_STAGE_VALIDATION;
    ctx->stage_start_percent = 0;
    ctx->stage_end_percent = 5;
    ctx->speed_window_start = ctx->operation_start_time;

    return ctx;
}

void progress_free(progress_context_t *ctx) {
    if (ctx) free(ctx);
}

int progress_advance_stage(progress_context_t *ctx, int new_stage) {
    if (!ctx) return PROGRESS_ERR_NULL;
    if (new_stage < 0 || new_stage > 4) return PROGRESS_ERR_INVALID;

    ctx->current_stage = new_stage;
    ctx->stage_start_percent = stage_ranges[new_stage][0];
    ctx->stage_end_percent = stage_ranges[new_stage][1];
    ctx->stage_start_time = time(NULL);

    // Reset bytes for this stage
    ctx->bytes_written = 0;
    ctx->current_file_percent = 0;
    memset(ctx->current_file, 0, PATH_MAX);

    return PROGRESS_OK;
}

int progress_get_percent(progress_context_t *ctx) {
    if (!ctx) return 0;

    // For extract stage, use bytes written
    if (ctx->current_stage == PROGRESS_STAGE_EXTRACT && ctx->total_bytes > 0) {
        double ratio = (double)ctx->bytes_written / ctx->total_bytes;
        int stage_progress = (int)(ratio * 75);  // Extract is 75% of total (20-95)
        if (stage_progress > 75) stage_progress = 75;
        return 20 + stage_progress;
    }

    // For other stages, use linear time-based progress within stage range
    int range = ctx->stage_end_percent - ctx->stage_start_percent;
    time_t elapsed = time(NULL) - ctx->stage_start_time;

    // Estimate progress - start fast and slow down to avoid jumping at end
    int stage_progress;
    if (elapsed == 0) {
        stage_progress = 0;
    } else if (elapsed < 5) {
        // First 5 seconds: 50% of range
        stage_progress = (elapsed * range) / (5 * 2);
    } else {
        // After 5 seconds: gradually approach end
        stage_progress = (range / 2) + ((elapsed - 5) * (range / 2)) / 10;
    }

    // Clamp to range
    if (stage_progress > range) stage_progress = range;
    if (stage_progress < 0) stage_progress = 0;

    return ctx->stage_start_percent + stage_progress;
}

double progress_get_speed_mbps(progress_context_t *ctx) {
    if (!ctx) return 0.0;

    time_t now = time(NULL);
    time_t elapsed = now - ctx->speed_window_start;

    // Reset window if > 30 seconds
    if (elapsed > 30) {
        ctx->speed_window_bytes = ctx->bytes_written;
        ctx->speed_window_start = now;
        return ctx->speed_mbps;
    }

    // Need at least 10 seconds of data for accurate speed
    if (elapsed < 10) return ctx->speed_mbps;

    uint64_t bytes = ctx->bytes_written - ctx->speed_window_bytes;
    double mbps = (bytes / (1024.0 * 1024.0)) / elapsed;

    // Exponential moving average: 0.7 * old + 0.3 * new
    if (ctx->speed_mbps < 0.01) {
        // First measurement
        ctx->speed_mbps = mbps;
    } else {
        ctx->speed_mbps = (ctx->speed_mbps * 0.7) + (mbps * 0.3);
    }

    ctx->speed_window_bytes = ctx->bytes_written;
    ctx->speed_window_start = now;

    return ctx->speed_mbps;
}

int progress_get_time_remaining_seconds(progress_context_t *ctx, int *out_seconds) {
    if (!ctx || !out_seconds) return PROGRESS_ERR_NULL;

    double speed = progress_get_speed_mbps(ctx);
    if (speed < 0.1) {
        *out_seconds = 0;
        return PROGRESS_OK;
    }

    uint64_t remaining = ctx->total_bytes - ctx->bytes_written;
    double remaining_mb = remaining / (1024.0 * 1024.0);
    double seconds = remaining_mb / speed;

    *out_seconds = (int)seconds;
    if (*out_seconds < 0) *out_seconds = 0;

    return PROGRESS_OK;
}

int progress_update_extract(progress_context_t *ctx, uint64_t bytes_written,
                           const char *current_file, int file_percent) {
    if (!ctx) return PROGRESS_ERR_NULL;

    ctx->bytes_written = bytes_written;
    ctx->current_file_percent = file_percent;

    if (current_file) {
        strncpy(ctx->current_file, current_file, PATH_MAX - 1);
        ctx->current_file[PATH_MAX - 1] = '\0';
    }

    return PROGRESS_OK;
}

int progress_format_message(progress_context_t *ctx, char *out_msg, size_t msg_size,
                           const char *operation_name) {
    if (!ctx || !out_msg) return PROGRESS_ERR_NULL;
    if (msg_size < 50) return PROGRESS_ERR_INVALID;

    int percent = progress_get_percent(ctx);
    double speed = progress_get_speed_mbps(ctx);
    int time_remaining = 0;
    progress_get_time_remaining_seconds(ctx, &time_remaining);

    // Format time remaining
    char time_str[32] = "calculating...";
    if (time_remaining > 0) {
        int hours = time_remaining / 3600;
        int minutes = (time_remaining % 3600) / 60;
        int seconds = time_remaining % 60;

        if (hours > 0) {
            snprintf(time_str, sizeof(time_str), "%d:%02d:%02d", hours, minutes, seconds);
        } else {
            snprintf(time_str, sizeof(time_str), "%d:%02d", minutes, seconds);
        }
    }

    // Build message
    if (ctx->current_stage == PROGRESS_STAGE_EXTRACT && ctx->current_file[0]) {
        snprintf(out_msg, msg_size, "%s: %s (%d%%) - %.1f MB/s - %s remaining",
                 operation_name, ctx->current_file, ctx->current_file_percent, speed, time_str);
    } else {
        snprintf(out_msg, msg_size, "%s... (%d%%) - %.1f MB/s - %s remaining",
                 operation_name, percent, speed, time_str);
    }

    return PROGRESS_OK;
}
