#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#include "hash_table_padded.h"

#define TABLE_SIZE 131071
#define _GNU_SOURCE

void loadManifest(HashTable* ht, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o manifest: %s\n", filename);
        exit(1);
    }

    char lineBuffer[256];
    int urlCount = 0;

    while (fgets(lineBuffer, sizeof(lineBuffer), file)) {
        lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';
        ht_insert(ht, lineBuffer);
        urlCount++;
    }
    printf("Manifest carregado: %d URLs\n", urlCount);
    fclose(file);
}

void processLog(HashTable* ht, const char* filename) {
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
                    #pragma omp atomic update
                    node->hit_count++;
                }
            }
        }
    }

    free(rawText);
    free(lineArray);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_log>\n", argv[0]);
        return 1;
    }

    struct timespec startTime, endTime;
    HashTable* ht = ht_create(TABLE_SIZE);

    loadManifest(ht, "cdn_data_logs/manifest.txt");

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    processLog(ht, argv[1]);
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    double elapsedSec = (endTime.tv_sec - startTime.tv_sec)
                      + (endTime.tv_nsec - startTime.tv_nsec) / 1e9;
    printf("\ntempo de processamento do log: %.4f segundos.\n", elapsedSec);

    ht_save_results(ht, "results.csv");
    ht_destroy(ht);

    return 0;
}

// gcc -fopenmp -O2 -Wall analyzer_par_atomic_padded.c hash_table_padded.c -o analyzer_par_atomic_padded