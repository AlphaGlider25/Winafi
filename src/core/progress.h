#ifndef WINAFI_PROGRESS_H
#define WINAFI_PROGRESS_H

typedef struct winafi_session winafi_session_t;
typedef void (*winafi_progress_callback_t)(int percent, const char *message, void *user_data);

typedef struct {
    winafi_progress_callback_t callback;
    void *user_data;
} progress_context_t;

// Register progress callback
void progress_set_callback(progress_context_t *ctx,
                         winafi_progress_callback_t callback,
                         void *user_data);

// Fire progress event
void progress_fire(progress_context_t *ctx, int percent, const char *message);

#endif
