#include "net.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TIMEOUT_S 30
#define MAX_STRING_BYTES  (512 * 1024)

/* GitHub releases API — update if project moves */
#define VERSION_CHECK_URL \
    "https://api.github.com/repos/AlphaGlider25/Winafi/releases/latest"

typedef struct {
    char  *data;
    size_t size;
    size_t cap;
} membuf_t;

static size_t membuf_write(char *ptr, size_t size, size_t nmemb, void *ud) {
    membuf_t *b = (membuf_t *)ud;
    size_t n = size * nmemb;
    if (b->size + n + 1 > MAX_STRING_BYTES) return 0; /* abort */
    if (b->size + n + 1 > b->cap) {
        size_t newcap = b->cap ? b->cap * 2 : 4096;
        while (newcap < b->size + n + 1) newcap *= 2;
        char *tmp = realloc(b->data, newcap);
        if (!tmp) return 0;
        b->data = tmp;
        b->cap  = newcap;
    }
    memcpy(b->data + b->size, ptr, n);
    b->size += n;
    b->data[b->size] = '\0';
    return n;
}

static size_t file_write(char *ptr, size_t size, size_t nmemb, void *ud) {
    return fwrite(ptr, size, nmemb, (FILE *)ud);
}

typedef struct {
    net_progress_cb_t cb;
    void *user_data;
} progress_ctx_t;

static int xfer_info(void *ud, curl_off_t dltotal, curl_off_t dlnow,
                     curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal; (void)ulnow;
    progress_ctx_t *ctx = (progress_ctx_t *)ud;
    if (ctx->cb)
        ctx->cb((long)dlnow, (long)dltotal, ctx->user_data);
    return 0;
}

void net_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void net_cleanup(void) {
    curl_global_cleanup();
}

int net_download_file(const char *url, const char *dest_path,
                      int timeout_seconds,
                      net_progress_cb_t progress_cb, void *user_data) {
    if (!url || !dest_path) return -1;

    FILE *f = fopen(dest_path, "wb");
    if (!f) return -1;

    CURL *c = curl_easy_init();
    if (!c) { fclose(f); return -1; }

    progress_ctx_t ctx = { progress_cb, user_data };
    long timeout = (long)(timeout_seconds > 0 ? timeout_seconds : DEFAULT_TIMEOUT_S);

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, file_write);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "Winify/4.0");
    if (progress_cb) {
        curl_easy_setopt(c, CURLOPT_XFERINFOFUNCTION, xfer_info);
        curl_easy_setopt(c, CURLOPT_XFERINFODATA, &ctx);
        curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
    }

    CURLcode res = curl_easy_perform(c);
    curl_easy_cleanup(c);
    fclose(f);
    return (res == CURLE_OK) ? 0 : -1;
}

char *net_fetch_string(const char *url, int timeout_seconds) {
    if (!url) return NULL;

    CURL *c = curl_easy_init();
    if (!c) return NULL;

    membuf_t buf = { NULL, 0, 0 };
    long timeout = (long)(timeout_seconds > 0 ? timeout_seconds : DEFAULT_TIMEOUT_S);

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, membuf_write);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "Winify/4.0");

    CURLcode res = curl_easy_perform(c);
    curl_easy_cleanup(c);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }
    return buf.data; /* caller must free() */
}

char *net_check_latest_version(void) {
    char *json = net_fetch_string(VERSION_CHECK_URL, 10);
    if (!json) return NULL;

    /* Parse "tag_name":"v4.1.0" from GitHub JSON */
    char *tag = strstr(json, "\"tag_name\"");
    if (!tag) { free(json); return NULL; }

    char *start = strchr(tag + 10, '"');
    if (!start) { free(json); return NULL; }
    start++;
    if (*start == 'v') start++; /* strip leading 'v' */

    char *end = strchr(start, '"');
    if (!end) { free(json); return NULL; }

    size_t len = (size_t)(end - start);
    char *ver = malloc(len + 1);
    if (!ver) { free(json); return NULL; }
    memcpy(ver, start, len);
    ver[len] = '\0';
    free(json);
    return ver; /* caller must free() */
}
