#include "iso_verify.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define HASH_BLOCK_SIZE (64 * 1024)  // 64KB chunks for efficiency

static const EVP_MD *hash_evp_md(hash_type_t type) {
    switch (type) {
        case HASH_MD5: return EVP_md5();
        case HASH_SHA256: return EVP_sha256();
        case HASH_SHA512: return EVP_sha512();
        default: return NULL;
    }
}

static size_t hash_digest_length(hash_type_t type) {
    switch (type) {
        case HASH_MD5: return 16;
        case HASH_SHA256: return 32;
        case HASH_SHA512: return 64;
        default: return 0;
    }
}

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
    const EVP_MD *md = hash_evp_md(type);
    size_t digest_len = hash_digest_length(type);
    if (!md || digest_len == 0) {
        fclose(f);
        return -1;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned char buffer[HASH_BLOCK_SIZE];
    size_t bytes;
    uint64_t current = 0;

    // Get file size for progress calculation
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long end_pos = ftell(f);
    if (end_pos < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -1;
    }
    uint64_t file_size = (uint64_t)end_pos;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fclose(f);
        return -1;
    }
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return -1;
    }

    while ((bytes = fread(buffer, 1, HASH_BLOCK_SIZE, f)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytes) != 1) {
            EVP_MD_CTX_free(ctx);
            fclose(f);
            return -1;
        }
        current += (uint64_t)bytes;
        if (progress_cb) {
            int percent = (file_size > 0) ? (int)(current * 100 / file_size) : 0;
            progress_cb(percent, user_data);
        }
    }
    if (ferror(f)) {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return -1;
    }
    unsigned int out_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &out_len) != 1 || out_len != digest_len) {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return -1;
    }
    EVP_MD_CTX_free(ctx);

    for (size_t i = 0; i < digest_len; i++) {
        sprintf(&result->hash_string[i * 2], "%02x", digest[i]);
    }
    result->hash_string[digest_len * 2] = '\0';

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
