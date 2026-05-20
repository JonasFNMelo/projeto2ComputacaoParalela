/*
 * analyzer_seq.c
 * Versao sequencial - baseline do projeto.
 * Le o log linha a linha e conta os acessos de cada URL.
 *
 * Uso: ./analyzer_seq <arquivo_log>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"

#define MANIFEST_FILE "manifest.txt"
#define RESULTS_FILE "results.csv"
#define TABLE_SIZE 131071
#define MAX_LINE 1024
#define MAX_URL 512

// le o manifest e popula a tabela hash
static long carregar_manifest(HashTable *ht)
{
    FILE *fp = fopen(MANIFEST_FILE, "r");
    if (!fp) { 
        perror("fopen manifest.txt"); 
        exit(1); 
    }

    char linha[MAX_URL];
    long total = 0;

    while (fgets(linha, sizeof(linha), fp)) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (linha[0] == '\0') {
            continue;
        }
        ht_put(ht, linha);
        total++;
    }

    fclose(fp);
    return total;
}

// extrai a URL de uma linha no formato:
// 127.0.0.1 - - [timestamp] "GET /url HTTP/1.1" 200 1500
static int extrair_url(const char *linha, char *url_out)
{
    const char *p = strchr(linha, '"');
    if (!p) {
        return 0;
    }
    p++;

    char metodo[16];
    if (sscanf(p, "%15s %511s", metodo, url_out) != 2) {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Uso: %s <arquivo_log>\n", argv[0]);
        return 1;
    }

    // printf("[1/3] Carregando manifest...\n");
    HashTable *ht = ht_create(TABLE_SIZE);
    long n_urls = carregar_manifest(ht); // carrega o manifest e conta quantas URLs tem pra carregar na hashtable
    printf("      %ld URLs carregadas.\n", n_urls);

    // 2. processa o log linha a linha
    // printf("[2/3] Processando %s...\n", argv[1]);

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    FILE *fp = fopen(argv[1], "r");
    if (!fp) { 
        perror("fopen log"); 
        return 1; 
    }

    char linha[MAX_LINE];
    char url[MAX_URL];
    long processadas = 0;

    while (fgets(linha, sizeof(linha), fp)) {
        if (!extrair_url(linha, url)) {
            continue;
        }

        CacheNode *no = ht_get(ht, url);
        if (no) {
            no->hit_count++; // (*no).hit_count++
            processadas++;
        }
    }
    fclose(fp);

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec  - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) * 1000000000.0;

    printf("      %ld linhas processadas em %.3f s\n", processadas, tempo);

    // printf("[3/3] Salvando %s...\n", RESULTS_FILE);
    ht_save_results(ht, RESULTS_FILE);

    ht_destroy(ht);
    // printf("Pronto!\n");
    return 0;
}