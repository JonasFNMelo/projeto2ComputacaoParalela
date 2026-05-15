#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void load_manifest(HashTable *ht, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror("manifest"); exit(1); }
    char buf[2048];
    while (fgets(buf, sizeof(buf), f)) {
        size_t n = strlen(buf);
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = 0;
        if (n > 0) ht_insert(ht, buf);
    }
    fclose(f);
}

static void load_lines(const char *path, char ***out_lines,
                       size_t *out_count, char **out_buf) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("log"); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (fread(buf, 1, sz, f) != (size_t)sz) { perror("fread"); exit(1); }
    buf[sz] = 0;
    fclose(f);

    size_t count = 0;
    for (long i = 0; i < sz; i++) if (buf[i] == '\n') count++;
    if (sz > 0 && buf[sz-1] != '\n') count++;

    char **lines = malloc(sizeof(char*) * count);
    size_t k = 0;
    lines[k++] = buf;
    for (long i = 0; i < sz; i++) {
        if (buf[i] == '\n') {
            buf[i] = 0;
            if (i + 1 < sz) lines[k++] = &buf[i+1];
        }
    }
    *out_lines = lines;
    *out_count = k;
    *out_buf = buf;
}

static int extract_url(const char *line, char *out, size_t outsz) {
    const char *q = strchr(line, '"');
    if (!q) return 0;
    q++;
    while (*q && *q != ' ') q++;
    if (*q != ' ') return 0;
    q++;
    size_t i = 0;
    while (*q && *q != ' ' && i + 1 < outsz) out[i++] = *q++;
    out[i] = 0;
    return i > 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "uso: %s <log.txt>\n", argv[0]); return 1; }

    HashTable *ht = ht_create(HT_DEFAULT_SIZE);
    load_manifest(ht, "manifest.txt");

    char **lines; size_t n; char *buf;
    load_lines(argv[1], &lines, &n, &buf);

    for (size_t i = 0; i < n; i++) {
        char url[2048];
        if (extract_url(lines[i], url, sizeof(url))) {
            CacheNode *node = ht_get(ht, url);
            if (node) node->hit_count++;
        }
    }

    ht_save_results(ht, "results.csv");
    free(lines); free(buf); ht_destroy(ht);
    return 0;
}
