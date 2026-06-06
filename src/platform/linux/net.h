#ifndef WINAFI_NET_H
#define WINAFI_NET_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Must be called once before any other net_* function. */
void net_init(void);

/* Call when done with all network operations. */
void net_cleanup(void);

/* Download URL to file at dest_path.
 * timeout_seconds: 0 = use default (30s).
 * progress_cb: called with (bytes_downloaded, total_bytes, user_data); may be NULL.
 * Returns 0 on success, -1 on error. */
typedef void (*net_progress_cb_t)(long downloaded, long total, void *user_data);
int net_download_file(const char *url, const char *dest_path,
                      int timeout_seconds,
                      net_progress_cb_t progress_cb, void *user_data);

/* Download URL to a heap-allocated NUL-terminated string.
 * Caller must free(). Returns NULL on error.
 * Aborts download if response exceeds 512 KiB. */
char *net_fetch_string(const char *url, int timeout_seconds);

/* Query GitHub releases API for latest version string (e.g. "4.1.0").
 * Caller must free(). Returns NULL on error or no network. */
char *net_check_latest_version(void);

#ifdef __cplusplus
}
#endif

#endif
