#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"

#define TABLE_SIZE 131071
#define _GNU_SOURCE

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

 // Processa o log linha a linha de forma sequencial, incrementando hit_count.
void processLog(HashTable* ht, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao abrir o log: %s\n", filename);
        return;
    }

    char line[1024];
    char url[1024];
    long lineCount = 0;

    while (fgets(line, sizeof(line), file)) {
        // pega o conteúdo entre o primeiro par de aspas: GET /path HTTP/1.1
        strtok(line, "\"");
        char* methodUrl = strtok(NULL, "\"");

        if (methodUrl != NULL) {
            char httpMethod[16];
            sscanf(methodUrl, "%s %s", httpMethod, url);

            CacheNode* node = ht_get(ht, url);
            if (node != NULL) node->hit_count++;
        }

        lineCount++;
        if (lineCount % 1000000 == 0) {
            printf("Processadas: %ld milhoes de linhas\n", lineCount / 1000000);
        }
    }

    printf("Log finalizado: %ld linhas no total\n", lineCount);
    fclose(file);
}

//É necessario passar o path do log como argumento para a main
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Erro, uso: %s <arquivo_log>\n", argv[0]);
        return 1;
    }

    struct timespec startTime, endTime;
    HashTable* ht = ht_create(TABLE_SIZE);

    //Inicio do calculo do tempo
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    loadManifest(ht, "cdn_data_logs/manifest.txt");
    processLog(ht, argv[1]);

    //Fim do calculo do tempo
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    double elapsedSec = (endTime.tv_sec - startTime.tv_sec)
                      + (endTime.tv_nsec - startTime.tv_nsec) / 1e9;
    printf("\ntempo de processamento do log: %.4f segundos.\n", elapsedSec);

    ht_save_results(ht, "results.csv");
    ht_destroy(ht);

    return 0;
}

// gcc -O2 -Wall analyzer_seq.c hash_table.c -o analyzer_seq
