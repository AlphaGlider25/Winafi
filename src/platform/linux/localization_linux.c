#include "localization_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 2048
#define MAX_KEY 128
#define MAX_VAL 512

typedef struct { char key[MAX_KEY]; char val[MAX_VAL]; } loc_entry_t;
static loc_entry_t *g_entries = NULL;
static int g_count = 0;

void loc_unload(void) {
    free(g_entries);
    g_entries = NULL;
    g_count = 0;
}

static int parse_lines(const char *content) {
    loc_unload();
    g_entries = calloc(MAX_ENTRIES, sizeof(loc_entry_t));
    if (!g_entries) return -1;

    const char *p = content;
    while (*p && g_count < MAX_ENTRIES) {
        // Skip comments and blank lines
        while (*p == '\r' || *p == '\n') p++;
        if (!*p) break;
        if (*p == '#' || *p == ';') { while (*p && *p != '\n') p++; continue; }

        const char *eq = strchr(p, '=');
        const char *nl = strchr(p, '\n');
        if (!eq || (nl && eq > nl)) { while (*p && *p != '\n') p++; continue; }

        size_t klen = (size_t)(eq - p);
        if (klen >= MAX_KEY) { while (*p && *p != '\n') p++; continue; }
        strncpy(g_entries[g_count].key, p, klen);
        g_entries[g_count].key[klen] = '\0';

        const char *vstart = eq + 1;
        size_t vlen = nl ? (size_t)(nl - vstart) : strlen(vstart);
        if (vlen >= MAX_VAL) vlen = MAX_VAL - 1;
        strncpy(g_entries[g_count].val, vstart, vlen);
        g_entries[g_count].val[vlen] = '\0';
        g_count++;

        p = nl ? nl + 1 : p + strlen(p);
    }
    return 0;
}

int loc_load_from_string(const char *content) {
    if (!content) return -1;
    return parse_lines(content);
}

int loc_load_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return -1;
    }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return -1;
    }
    buf[sz] = '\0';
    fclose(f);
    int rc = parse_lines(buf);
    free(buf);
    return rc;
}

const char *loc_get_string(const char *key) {
    if (!key) return "";
    if (!g_entries) return key;
    for (int i = 0; i < g_count; i++)
        if (strcmp(g_entries[i].key, key) == 0) return g_entries[i].val;
    return key; // fallback: return the key itself
}
