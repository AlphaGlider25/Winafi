#ifndef WINAFI_LOCALIZATION_LINUX_H
#define WINAFI_LOCALIZATION_LINUX_H

/* Load locale key=value pairs from a NUL-terminated string. Returns 0 on success. */
int loc_load_from_string(const char *content);

/* Load locale from a file path. Returns 0 on success. */
int loc_load_from_file(const char *path);

/* Get translated string for key. Returns key itself if not found (never NULL). */
const char *loc_get_string(const char *key);

/* Free loaded locale data. */
void loc_unload(void);

#endif
