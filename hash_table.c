#include "hash_table.h"

// --- Constantes Internas ---

// Função de Hash (djb2)
// Converte uma string (URL) em um índice para a tabela.
// Fonte: http://www.cse.yorku.ca/~oz/hash.html
static size_t hash_djb2(const char* str, size_t size)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % size;
}

// Cria um novo nó (função auxiliar interna)
static CacheNode* criar_no(const char* url)
{
    CacheNode* node = (CacheNode*)malloc(sizeof(CacheNode));
    if (!node) {
        perror("Erro ao alocar CacheNode");
        exit(EXIT_FAILURE);
    }

    node->url = (char*)malloc(strlen(url) + 1);
    if (!node->url) {
        perror("Erro ao alocar string da URL");
        free(node);
        exit(EXIT_FAILURE);
    }
    strcpy(node->url, url);

    node->hit_count = 0;
    node->next = NULL;
    return node;
}

// --- Implementação da API Pública ---

HashTable* ht_create(size_t size)
{
    if (size < 1) {
        fprintf(stderr, "Tamanho da tabela deve ser ao menos 1\n");
        return NULL;
    }

    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) {
        perror("Erro ao alocar HashTable");
        return NULL;
    }

    // calloc inicializa todos os ponteiros (buckets) como NULL
    ht->table = (CacheNode**)calloc(size, sizeof(CacheNode*));
    if (!ht->table) {
        perror("Erro ao alocar buckets da tabela");
        free(ht);
        return NULL;
    }

    ht->size = size;
    return ht;
}

void ht_destroy(HashTable* ht)
{
    if (!ht) return;

    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* cur = ht->table[i];
        while (cur) {
            CacheNode* prox = cur->next;
            free(cur->url);
            free(cur);
            cur = prox;
        }
    }

    free(ht->table);
    free(ht);
}

void ht_put(HashTable* ht, const char* url)
{
    if (!ht || !url) return;

    size_t idx = hash_djb2(url, ht->size);

    // Verifica se já existe (não duplica)
    CacheNode* cur = ht->table[idx];
    while (cur) {
        if (strcmp(cur->url, url) == 0) return;
        cur = cur->next;
    }

    // Insere no início da lista do bucket
    CacheNode* novo = criar_no(url);
    novo->next = ht->table[idx];
    ht->table[idx] = novo;
}

CacheNode* ht_get(HashTable* ht, const char* url)
{
    if (!ht || !url) return NULL;

    size_t idx = hash_djb2(url, ht->size);

    CacheNode* cur = ht->table[idx];
    while (cur) {
        if (strcmp(cur->url, url) == 0) return cur;
        cur = cur->next;
    }

    return NULL;
}

size_t ht_bucket_of(HashTable* ht, const char* url)
{
    return hash_djb2(url, ht->size);
}

void ht_save_results(HashTable* ht, const char* filename)
{
    if (!ht || !filename) return;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo de resultados");
        return;
    }

    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* cur = ht->table[i];
        while (cur) {
            fprintf(fp, "%s,%ld\n", cur->url, cur->hit_count);
            cur = cur->next;
        }
    }

    fclose(fp);
}

void ht_print(HashTable* ht)
{
    if (!ht) return;
    printf("--- Estado da Tabela Hash (Size: %zu) ---\n", ht->size);
    for (size_t i = 0; i < ht->size; i++) {
        if (!ht->table[i]) continue;
        printf("Bucket[%zu]: ", i);
        CacheNode* cur = ht->table[i];
        while (cur) {
            printf("[\"%s\" (%ld)] -> ", cur->url, cur->hit_count);
            cur = cur->next;
        }
        printf("NULL\n");
    }
    printf("-----------------------------------------\n");
}
