#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

#define HT_DEFAULT_SIZE 131071

typedef struct CacheNode {
    char *url;
    long hit_count;
    struct CacheNode *next;
#ifdef PADDED
    long padding[5];
#endif
} CacheNode;

typedef struct HashTable {
    CacheNode **buckets;
    size_t size;
} HashTable;

HashTable *ht_create(size_t size);
void ht_insert(HashTable *ht, const char *url);
CacheNode *ht_get(HashTable *ht, const char *url);
void ht_save_results(HashTable *ht, const char *filename);
void ht_destroy(HashTable *ht);
size_t ht_hash(const char *url, size_t size);

#endif
