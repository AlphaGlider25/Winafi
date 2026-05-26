#include "iso_verify.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define HASH_BLOCK_SIZE (64 * 1024)  // 64KB chunks for efficiency

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
                     iso_hash_progress_cb_t progress_cb, void *user_data) {
    if (!iso_path || !result) {
        return -1;
    }

    FILE *f = fopen(iso_path, "rb");
    if (!f) {
        return -1;
    }

    result->type = type;
    unsigned char digest[SHA512_DIGEST_LENGTH];
    unsigned char buffer[HASH_BLOCK_SIZE];
    size_t bytes;
    uint64_t current = 0;

    // Get file size for progress calculation
    fseek(f, 0, SEEK_END);
    uint64_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    switch (type) {
        case HASH_MD5: {
            MD5_CTX ctx;
            MD5_Init(&ctx);
            while ((bytes = fread(buffer, 1, HASH_BLOCK_SIZE, f)) > 0) {
                MD5_Update(&ctx, buffer, bytes);
                current += bytes;
                if (progress_cb) {
                    int percent = (file_size > 0) ? (int)(current * 100 / file_size) : 0;
                    progress_cb(percent, user_data);
                }
            }
            MD5_Final(digest, &ctx);
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
                sprintf(&result->hash_string[i*2], "%02x", digest[i]);
            }
            break;
        }
        case HASH_SHA256: {
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            while ((bytes = fread(buffer, 1, HASH_BLOCK_SIZE, f)) > 0) {
                SHA256_Update(&ctx, buffer, bytes);
                current += bytes;
                if (progress_cb) {
                    int percent = (file_size > 0) ? (int)(current * 100 / file_size) : 0;
                    progress_cb(percent, user_data);
                }
            }
            SHA256_Final(digest, &ctx);
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                sprintf(&result->hash_string[i*2], "%02x", digest[i]);
            }
            break;
        }
        case HASH_SHA512: {
            SHA512_CTX ctx;
            SHA512_Init(&ctx);
            while ((bytes = fread(buffer, 1, HASH_BLOCK_SIZE, f)) > 0) {
                SHA512_Update(&ctx, buffer, bytes);
                current += bytes;
                if (progress_cb) {
                    int percent = (file_size > 0) ? (int)(current * 100 / file_size) : 0;
                    progress_cb(percent, user_data);
                }
            }
            SHA512_Final(digest, &ctx);
            for (int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
                sprintf(&result->hash_string[i*2], "%02x", digest[i]);
            }
            break;
        }
        default:
            fclose(f);
            return -1;
    }

    fclose(f);
    result->verified = -1;  // not yet verified
    return 0;
}

/**
 * Verify computed hash against expected hash.
 * Case-insensitive comparison of hash strings.
 *
 * iso_path: path to ISO file
 * expected: pointer to iso_hash_t with expected hash value
 *
 * Returns: 1 if hashes match, 0 if mismatch, -1 on error
 */
int iso_verify_hash(const char *iso_path, const iso_hash_t *expected) {
    if (!iso_path || !expected) {
        return -1;
    }

    iso_hash_t computed;
    int ret = iso_compute_hash(iso_path, expected->type, &computed, NULL, NULL);
    if (ret != 0) {
        return -1;
    }

    // Case-insensitive string comparison
    if (strcasecmp(computed.hash_string, expected->hash_string) == 0) {
        return 1;  // Match
    }
    return 0;  // Mismatch
}

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
int iso_detect_hash_from_file(const char *iso_dir, iso_hash_t *hashes, int *count) {
    if (!iso_dir || !hashes || !count) {
        return -1;
    }

    // TODO: Implement in Task 4B-4
    *count = 0;
    return 0;
}
