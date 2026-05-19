#ifndef HASH_TABLE_PADDED_H
#define HASH_TABLE_PADDED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Variante do CacheNode com padding adicional, conforme PDF seção 8.6
 * (Experimento C - False Sharing).
 *
 * O objetivo é "afastar" os hit_count na memória, reduzindo a probabilidade
 * de que dois contadores (acessados por threads distintas) caiam na mesma
 * linha de cache (tipicamente 64 bytes).
 */
typedef struct CacheNode {
    char* url;
    long hit_count;
    struct CacheNode* next;
    long padding[5]; // padding conforme exigido pelo enunciado
} CacheNode;

typedef struct {
    size_t size;
    CacheNode** table;
} HashTable;

HashTable* ht_create(size_t size);
void       ht_destroy(HashTable* ht);
void       ht_insert(HashTable* ht, const char* url);
CacheNode* ht_get(HashTable* ht, const char* url);
void       ht_save_results(HashTable* ht, const char* filename);
size_t     ht_get_hash(HashTable* ht, const char* url);
void       ht_print(HashTable* ht);

#endif // HASH_TABLE_PADDED_H
