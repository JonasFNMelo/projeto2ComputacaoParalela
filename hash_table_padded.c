#include "hash_table_padded.h"

static size_t hashDjb2(const char* str, size_t size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

static CacheNode* createNode(const char* url) {
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
    // padding não precisa ser inicializado (não é lido)
    return node;
}

HashTable* ht_create(size_t size) {
    if (size < 1) {
        fprintf(stderr, "Tamanho da tabela deve ser ao menos 1\n");
        return NULL;
    }

    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) {
        perror("Erro ao alocar HashTable");
        return NULL;
    }

    ht->table = (CacheNode**)calloc(size, sizeof(CacheNode*));
    if (!ht->table) {
        perror("Erro ao alocar buckets da tabela");
        free(ht);
        return NULL;
    }

    ht->size = size;
    return ht;
}

void ht_destroy(HashTable* ht) {
    if (!ht) return;
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* current = ht->table[i];
        while (current) {
            CacheNode* next = current->next;
            free(current->url);
            free(current);
            current = next;
        }
    }
    free(ht->table);
    free(ht);
}

void ht_insert(HashTable* ht, const char* url) {
    if (!ht || !url) return;
    size_t index = hashDjb2(url, ht->size);

    CacheNode* current = ht->table[index];
    while (current) {
        if (strcmp(current->url, url) == 0) return;
        current = current->next;
    }

    CacheNode* newNode = createNode(url);
    newNode->next = ht->table[index];
    ht->table[index] = newNode;
}

CacheNode* ht_get(HashTable* ht, const char* url) {
    if (!ht || !url) return NULL;
    size_t index = hashDjb2(url, ht->size);

    CacheNode* current = ht->table[index];
    while (current) {
        if (strcmp(current->url, url) == 0) return current;
        current = current->next;
    }
    return NULL;
}

void ht_save_results(HashTable* ht, const char* filename) {
    if (!ht || !filename) {
        fprintf(stderr, "Erro: tabela ou nome de arquivo nulo ao salvar resultados.\n");
        return;
    }

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Erro ao abrir arquivo de resultados");
        return;
    }

    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* current = ht->table[i];
        while (current) {
            fprintf(fp, "%s,%ld\n", current->url, current->hit_count);
            current = current->next;
        }
    }
    fclose(fp);
}

size_t ht_get_hash(HashTable* ht, const char* url) {
    if (!ht || !url) {
        fprintf(stderr, "Erro em ht_get_hash.\n");
        exit(EXIT_FAILURE);
    }
    return hashDjb2(url, ht->size);
}

void ht_print(HashTable* ht) {
    if (!ht) return;
    printf("--- Estado da Tabela Hash (Size: %zu) ---\n", ht->size);
    for (size_t i = 0; i < ht->size; i++) {
        printf("Bucket[%zu]: ", i);
        CacheNode* current = ht->table[i];
        if (!current) {
            printf("~ VAZIO ~\n");
            continue;
        }
        while (current) {
            printf("[\"%s\" (%ld)] -> ", current->url, current->hit_count);
            current = current->next;
        }
        printf("NULL\n");
    }
    printf("-----------------------------------------\n");
}
