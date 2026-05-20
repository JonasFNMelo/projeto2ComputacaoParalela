#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"
#define MANIFEST_FILE  "manifest.txt"
#define RESULTS_FILE   "results.csv"
#define TABLE_SIZE 131071   // primo aprox de 2^17 
#define MAX_LINE 1024
#define MAX_URL 512

typedef struct { // vetor de strings que guardar o log na memoria
    char **linhas;
    long total;
    long capacidade;
} VetorLinhas;

static long carregar_manifest(HashTable *ht) // le o manifest e passa pra tabela hash
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
        if (linha[0] == '\0') continue;
        ht_put(ht, linha);
        total++;
    }
    fclose(fp);
    return total;
}

// extrai a URL de uma linha de log no formato Apache/Nginx:
static int extrair_url(const char *linha, char *url_out)
{
    const char *p = strchr(linha, '"'); // procura a primeira aspas do "GET /url
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

// le o arquivo de log inteiro para um vetor de strings em memoria
static VetorLinhas *ler_log(const char *arquivo) //le o arquivo log em um vetor de strings em memoria para paralelizar depois
{
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        perror("fopen log");
        exit(1);
    }
    VetorLinhas *v = (VetorLinhas *)malloc(sizeof(VetorLinhas));
    if (!v) { 
        perror("malloc VetorLinhas"); 
        exit(1); 
    }
    v->capacidade = 12 * 1024 * 1024;
    v->total = 0;
    v->linhas = (char **)malloc(sizeof(char *) * v->capacidade);
    if (!v->linhas) { 
        perror("malloc linhas"); 
        exit(1);
    }
    char buf[MAX_LINE];
    while (fgets(buf, sizeof(buf), fp)) {
        if (v->total == v->capacidade) {
            v->capacidade *= 2;
            v->linhas = (char **)realloc(v->linhas, sizeof(char *) * v->capacidade);
            if (!v->linhas) { 
                perror("realloc"); 
                exit(1); 
            }
        }
        v->linhas[v->total++] = strdup(buf);
    }
    fclose(fp);
    return v;
}

// libera a memoria do vetor de linhas
static void liberar_vetor(VetorLinhas *v)
{
    for (long i = 0; i < v->total; i++){
        free(v->linhas[i]);
    }
    free(v->linhas);
    free(v);
}
#endif // COMMON_H
