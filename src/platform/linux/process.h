#ifndef WINAFI_PROCESS_H
#define WINAFI_PROCESS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 if devnode (e.g. "/dev/sdb") is currently open by any process,
   0 if not in use, -1 on error (e.g. permission denied reading /proc). */
int process_device_in_use(const char *devnode);

/* Fills out[] with up to max_out process names that have devnode open.
   Returns the number of names written (0 if none, -1 on error).
   Each name is NUL-terminated, max 256 bytes. */
int process_list_users(const char *devnode, char out[][256], int max_out);

/* Returns 1 if the current process is running as root (uid == 0), 0 otherwise. */
int process_is_root(void);

#ifdef __cplusplus
}
#endif
#endif
