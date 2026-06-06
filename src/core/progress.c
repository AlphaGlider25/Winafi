#include "progress.h"
#include <string.h>

void progress_set_callback(progress_context_t *ctx,
                         winafi_progress_callback_t callback,
                         void *user_data) {
    if (!ctx) return;
    ctx->callback = callback;
    ctx->user_data = user_data;
}

void progress_fire(progress_context_t *ctx, int percent, const char *message) {
    if (!ctx || !ctx->callback) return;

    // Clamp percent to 0-100
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    ctx->callback(percent, message, ctx->user_data);
}
