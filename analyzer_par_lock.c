#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#include "hash_table.h"

#define _GNU_SOURCE
#define TABLE_SIZE 131071

//Le o manifesto e insere cada URL na tabela hash.
void loadManifest(HashTable* ht, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o manifest: %s\n", filename);
        exit(1);
    }
    //Instancia do buffer e do contador
    char lineBuffer[256];
    int urlCount = 0;

    while (fgets(lineBuffer, sizeof(lineBuffer), file)) {
        // remove \r e \n do final
        lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';
        //Adciona a url no hash
        ht_insert(ht, lineBuffer);
        urlCount++;
    }
    printf("Manifest carregado: %d URLs\n", urlCount);
    fclose(file);
}

//Cria um array com um omp_lock_t por bucket da tabela hash.
omp_lock_t* createBucketLocks(size_t tableSize) {
    //Ponteiro do primeiri lock
    omp_lock_t* locks = malloc(sizeof(omp_lock_t) * tableSize);
    if (locks == NULL) {
        fprintf(stderr, "Erro criação locks\n");
        exit(1);
    }

    for (size_t i = 0; i < tableSize; i++) {
        omp_init_lock(&locks[i]);
    }
    return locks;
}

//Libera o array de locks por bucket.
void destroyBucketLocks(omp_lock_t* locks, size_t tableSize) {
    for (size_t i = 0; i < tableSize; i++) {
        omp_destroy_lock(&locks[i]);
    }
    free(locks);
}

//Processa o log em paralelo usando um lock por bucket, threads só competem quando acessam o mesmo bucket.
void processLog(HashTable* ht, const char* filename, omp_lock_t* locks) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o log: %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* rawText = malloc(fileSize + 1);
    if (rawText == NULL) {
        printf("Erro ao alocar memória, log: %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fread(rawText, 1, fileSize, file) == (size_t)fileSize) {
        rawText[fileSize] = '\0';
        fclose(file);
    } else {
        printf("Erro no carregamento do arquivo em memória.\n");
        free(rawText);
        fclose(file);
        exit(1);
    }

    long lineCount = 0;
    for (long i = 0; i < fileSize; i++) {
        if (rawText[i] == '\n') lineCount++;
    }
    if (fileSize > 0 && rawText[fileSize - 1] != '\n') {
        lineCount++;
    }

    char** lineArray = malloc(lineCount * sizeof(char*));
    long lineIdx = 0;
    lineArray[lineIdx++] = rawText;

    for (long i = 0; i < fileSize; i++) {
        if (rawText[i] == '\n') {
            rawText[i] = '\0';
            if (i + 1 < fileSize && lineIdx < lineCount) {
                lineArray[lineIdx++] = &rawText[i + 1];
            }
        }
    }

    printf("Log carregado: %ld linhas\n", lineCount);

    #pragma omp parallel
    {
        #pragma omp single
        printf("Threads ativas nesta região: %d\n", omp_get_num_threads());

        #pragma omp for
        for (long j = 0; j < lineIdx; j++) {
            char url[512];
            char* saveptr;

            strtok_r(lineArray[j], "\"", &saveptr);
            char* methodUrl = strtok_r(NULL, "\"", &saveptr);

            if (methodUrl != NULL) {
                char httpMethod[16];
                sscanf(methodUrl, "%s %s", httpMethod, url);

                CacheNode* node = ht_get(ht, url);
                if (node != NULL) {
                    size_t bucket = ht_get_hash(ht, url);
                    // Threads em buckets diferentes não bloqueiam umas às outras
                    omp_set_lock(&locks[bucket]);
                    node->hit_count++;
                    omp_unset_lock(&locks[bucket]);
                }
            }
        }
    }

    free(rawText);
    free(lineArray);
}

//Uso: ./analyzer_par_lock <arquivo_log>
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_log>\n", argv[0]);
        return 1;
    }

    struct timespec startTime, endTime;
    HashTable* ht = ht_create(TABLE_SIZE);
    omp_lock_t* locks = createBucketLocks(TABLE_SIZE);

    //Inicio do calculo do tempo
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    loadManifest(ht, "cdn_data_logs/manifest.txt");
    processLog(ht, argv[1], locks);

    //Fim do calculo do tempo
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    double elapsedSec = (endTime.tv_sec - startTime.tv_sec)
                      + (endTime.tv_nsec - startTime.tv_nsec) / 1e9;
    printf("\ntempo de processamento do log: %.4f segundos.\n", elapsedSec);

    destroyBucketLocks(locks, TABLE_SIZE);
    ht_save_results(ht, "results.csv");
    ht_destroy(ht);

    return 0;
}

// gcc -fopenmp -O2 -Wall analyzer_par_lock.c hash_table.c -o analyzer_par_lock