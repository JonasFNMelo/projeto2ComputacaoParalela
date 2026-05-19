#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Estruturas de Dados ---

/*
 * Nó da Tabela Hash (também usado para encadeamento em caso de colisão).
 * Os nomes dos campos (url, hit_count, next) são mantidos pois fazem parte
 * da especificação do projeto (PDF seção 4) e são acessados diretamente
 * pelos analyzers.
 */
typedef struct CacheNode {
    char* url;                // Chave
    long hit_count;           // Valor (contador atualizado concorrentemente)
    struct CacheNode* next;   // Próximo nó em caso de colisão
} CacheNode;

/*
 * Estrutura principal da Tabela Hash.
 */
typedef struct {
    size_t size;          // Número de buckets
    CacheNode** table;    // Array de ponteiros para CacheNode
} HashTable;


// --- Interface Pública (API) ---
// Os nomes ht_create / ht_insert / ht_get / ht_save_results são exigidos
// pelo enunciado (PDF seção 4.1) e não podem ser alterados.

HashTable* ht_create(size_t size);
void       ht_destroy(HashTable* ht);
void       ht_insert(HashTable* ht, const char* url);
CacheNode* ht_get(HashTable* ht, const char* url);
void       ht_save_results(HashTable* ht, const char* filename);

/*
 * Retorna o índice (bucket) calculado para uma URL.
 * Usado pelo analyzer_par_lock para selecionar o lock correspondente.
 */
size_t     ht_get_hash(HashTable* ht, const char* url);

/* Função auxiliar de depuração. */
void       ht_print(HashTable* ht);

#endif // HASH_TABLE_H
