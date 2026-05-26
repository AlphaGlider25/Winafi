#ifndef RUFUS_PROGRESS_H
#define RUFUS_PROGRESS_H

typedef struct rufus_session rufus_session_t;
typedef void (*rufus_progress_callback_t)(int percent, const char *message, void *user_data);

typedef struct {
    rufus_progress_callback_t callback;
    void *user_data;
} progress_context_t;

// Register progress callback
void progress_set_callback(progress_context_t *ctx,
                         rufus_progress_callback_t callback,
                         void *user_data);

// Fire progress event
void progress_fire(progress_context_t *ctx, int percent, const char *message);

#endif
