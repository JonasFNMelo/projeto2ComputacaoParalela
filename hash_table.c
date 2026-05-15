#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* djb2 */
size_t ht_hash(const char *url, size_t size) {
    unsigned long h = 5381;
    int c;
    while ((c = *url++))
        h = ((h << 5) + h) + c;
    return h % size;
}

HashTable *ht_create(size_t size) {
    HashTable *ht = malloc(sizeof(HashTable));
    ht->size = size;
    ht->buckets = calloc(size, sizeof(CacheNode *));
    return ht;
}

void ht_insert(HashTable *ht, const char *url) {
    size_t idx = ht_hash(url, ht->size);
    CacheNode *node = malloc(sizeof(CacheNode));
    node->url = strdup(url);
    node->hit_count = 0;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
}

CacheNode *ht_get(HashTable *ht, const char *url) {
    size_t idx = ht_hash(url, ht->size);
    CacheNode *node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->url, url) == 0)
            return node;
        node = node->next;
    }
    return NULL;
}

void ht_save_results(HashTable *ht, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("fopen"); return; }
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode *node = ht->buckets[i];
        while (node) {
            fprintf(f, "%s,%ld\n", node->url, node->hit_count);
            node = node->next;
        }
    }
    fclose(f);
}

void ht_destroy(HashTable *ht) {
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode *node = ht->buckets[i];
        while (node) {
            CacheNode *next = node->next;
            free(node->url);
            free(node);
            node = next;
        }
    }
    free(ht->buckets);
    free(ht);
}
