#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Estruturas de Dados ---

/*
 * Nó da Tabela Hash (também usado para encadeamento em caso de colisão)
 */
typedef struct CacheNode {
    char* url;                // Chave
    long hit_count;           // Valor (o contador que será incrementado)
    struct CacheNode* next;   // Ponteiro para o próximo nó em caso de colisão

#ifdef USE_PADDING
    // padding adicionado para o Experimento C (false sharing)
    // força cada nó a ocupar uma linha de cache completa (64 bytes)
    long padding[5];
#endif
} CacheNode;

/*
 * Estrutura principal da Tabela Hash
 */
typedef struct {
    size_t size;          // Tamanho total da tabela (número de "buckets")
    CacheNode** table;    // O array de ponteiros para CacheNode (os "buckets")
} HashTable;


// --- Interface Pública (API) ---

/*
 * Cria uma nova Tabela Hash com um tamanho (size) específico.
 */
HashTable* ht_create(size_t size);

/*
 * Libera toda a memória associada à Tabela Hash.
 */
void ht_destroy(HashTable* ht);

/*
 * Insere um novo par (url, hit_count) na tabela.
 */
void ht_put(HashTable* ht, const char* url);

/*
 * Busca um nó na Tabela Hash pela URL.
 */
CacheNode* ht_get(HashTable* ht, const char* url);

/*
 * Salva os resultados (URL, hit_count) da tabela em um arquivo CSV.
 */
void ht_save_results(HashTable* ht, const char* filename);

/*
 * Função de depuração: Imprime o estado da tabela.
 */
void ht_print(HashTable* ht);

/*
 * Retorna o índice do bucket de uma URL.
 * Usada pelo analyzer_par_lock para saber qual lock adquirir.
 */
size_t ht_bucket_of(HashTable* ht, const char* url);


#endif // HASH_TABLE_H
