#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_ENTRIES 256
#define MAX_KEY_LEN  128
#define MAX_VAL_LEN  512

typedef struct { char key[MAX_KEY_LEN]; char val[MAX_VAL_LEN]; } entry_t;

struct settings {
    char path[1024];
    entry_t entries[MAX_ENTRIES];
    int count;
    int dirty;
};

static void ensure_dir(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *p = strrchr(tmp, '/');
    if (p) { *p = '\0'; mkdir(tmp, 0700); }
}

static void get_settings_path(char *buf, size_t n) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0])
        snprintf(buf, n, "%s/Winafi/winafi.ini", xdg);
    else {
        const char *home = getenv("HOME");
        snprintf(buf, n, "%s/.config/Winafi/winafi.ini", home ? home : "/tmp");
    }
}

static void load(settings_t *s) {
    FILE *f = fopen(s->path, "r");
    if (!f) return;
    char line[MAX_KEY_LEN + MAX_VAL_LEN + 4];
    while (fgets(line, (int)sizeof(line), f) && s->count < MAX_ENTRIES) {
        char *eq = strchr(line, '=');
        if (!eq || line[0] == '#' || line[0] == '[') continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        val[strcspn(val, "\r\n")] = '\0';
        snprintf(s->entries[s->count].key, MAX_KEY_LEN, "%.*s", MAX_KEY_LEN - 1, key);
        snprintf(s->entries[s->count].val, MAX_VAL_LEN, "%.*s", MAX_VAL_LEN - 1, val);
        s->count++;
    }
    fclose(f);
}

static int flush(settings_t *s) {
    if (!s->dirty) return 0;
    FILE *f = fopen(s->path, "w");
    if (!f) return -1;
    fprintf(f, "[Winafi]\n");
    for (int i = 0; i < s->count; i++)
        fprintf(f, "%s=%s\n", s->entries[i].key, s->entries[i].val);
    fclose(f);
    s->dirty = 0;
    return 0;
}

settings_t *settings_open(void) {
    settings_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    get_settings_path(s->path, sizeof(s->path));
    ensure_dir(s->path);
    load(s);
    return s;
}

int settings_close(settings_t *s) {
    if (!s) return 0;
    int rc = flush(s);
    free(s);
    return rc;
}

static entry_t *find_entry(settings_t *s, const char *key) {
    for (int i = 0; i < s->count; i++)
        if (strcmp(s->entries[i].key, key) == 0) return &s->entries[i];
    return NULL;
}

static entry_t *find_or_create(settings_t *s, const char *key) {
    entry_t *e = find_entry(s, key);
    if (e) return e;
    if (s->count >= MAX_ENTRIES) return NULL;
    e = &s->entries[s->count++];
    strncpy(e->key, key, MAX_KEY_LEN - 1);
    e->key[MAX_KEY_LEN - 1] = '\0';
    e->val[0] = '\0';
    return e;
}

int settings_get_string(settings_t *s, const char *key, char *out, size_t n, const char *def) {
    if (!s || !key || !out || n == 0) return -1;
    entry_t *e = find_entry(s, key);
    const char *src = e ? e->val : (def ? def : "");
    snprintf(out, n, "%s", src);
    return 0;
}

void settings_set_string(settings_t *s, const char *key, const char *val) {
    if (!s || !key) return;
    entry_t *e = find_or_create(s, key);
    if (!e) return;
    strncpy(e->val, val ? val : "", MAX_VAL_LEN - 1);
    e->val[MAX_VAL_LEN - 1] = '\0';
    s->dirty = 1;
}

int settings_get_int(settings_t *s, const char *key, int def) {
    if (!s || !key) return def;
    entry_t *e = find_entry(s, key);
    return e ? atoi(e->val) : def;
}

void settings_set_int(settings_t *s, const char *key, int val) {
    if (!s || !key) return;
    entry_t *e = find_or_create(s, key);
    if (!e) return;
    snprintf(e->val, MAX_VAL_LEN, "%d", val);
    s->dirty = 1;
}

int settings_get_bool(settings_t *s, const char *key, int def) {
    return settings_get_int(s, key, def) != 0;
}

void settings_set_bool(settings_t *s, const char *key, int val) {
    settings_set_int(s, key, val ? 1 : 0);
}
