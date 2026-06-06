#ifndef WINAFI_ISO_VERIFY_H
#define WINAFI_ISO_VERIFY_H

#include <stdint.h>

/**
 * Hash algorithm types supported
 */
typedef enum {
    HASH_MD5 = 0,
    HASH_SHA256 = 1,
    HASH_SHA512 = 2,
} hash_type_t;

/**
 * Result structure for hash operations
 */
typedef struct {
    hash_type_t type;              // Hash type used
    char hash_string[129];         // Max SHA512 is 128 hex chars + null
    int verified;                  // 1=match, 0=mismatch, -1=error/not verified
} iso_hash_t;

/**
 * Progress callback for hash computation
 * percent: 0-100 progress percentage
 * user_data: opaque user-provided context
 */
typedef void (*iso_hash_progress_cb_t)(int percent, void *user_data);

/**
 * Compute hash of ISO file.
 * Calls progress callback with percent (0-100) during computation.
 *
 * iso_path: path to ISO file
 * type: hash algorithm to use (HASH_MD5, HASH_SHA256, HASH_SHA512)
 * result: pointer to iso_hash_t structure to fill with results
 * progress_cb: optional progress callback (NULL if not needed)
 * user_data: opaque context passed to progress callback
 *
 * Returns: 0 on success, -1 on error
 */
int iso_compute_hash(const char *iso_path, hash_type_t type,
                     iso_hash_t *result,
                     iso_hash_progress_cb_t progress_cb, void *user_data);

/**
 * Verify computed hash against expected hash.
 * Case-insensitive comparison of hash strings.
 *
 * iso_path: path to ISO file
 * expected: pointer to iso_hash_t with expected hash value
 *
 * Returns: 1 if hashes match, 0 if mismatch, -1 on error
 */
int iso_verify_hash(const char *iso_path, const iso_hash_t *expected);

/**
 * Detect hash files in directory (e.g., file.iso.sha256, file.iso.md5).
 * Searches for common hash file formats associated with the ISO.
 *
 * iso_dir: directory containing the ISO and potential hash files
 * hashes: array of iso_hash_t structures to fill
 * count: pointer to count, returns number of hashes found
 *
 * Returns: count of hashes found, -1 on error
 */
int iso_detect_hash_from_file(const char *iso_dir, iso_hash_t *hashes, int *count);

#endif
