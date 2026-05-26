#ifndef RUFUS_SETTINGS_H
#define RUFUS_SETTINGS_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct settings settings_t;

settings_t *settings_open(void);
/* Returns 0 on success, -1 if settings could not be flushed to disk. */
int settings_close(settings_t *s);

int  settings_get_string(settings_t *s, const char *key, char *out, size_t out_size, const char *default_val);
void settings_set_string(settings_t *s, const char *key, const char *val);

int  settings_get_int(settings_t *s, const char *key, int default_val);
void settings_set_int(settings_t *s, const char *key, int val);

int  settings_get_bool(settings_t *s, const char *key, int default_val);
void settings_set_bool(settings_t *s, const char *key, int val);

#ifdef __cplusplus
}
#endif

#endif
