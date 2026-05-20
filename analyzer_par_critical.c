/*
 * analyzer_par_critical.c
 * Versao paralela com #pragma omp critical.
 *
 * Igual ao atomic, mas o incremento e protegido por uma regiao
 * critica global: so uma thread por vez pode incrementar qualquer
 * contador, mesmo que sejam URLs diferentes.
 *
 * Isso causa muita serializacao, especialmente no log concorrente.
 * O objetivo e medir o custo desse overhead (System time alto).
 *
 * Uso: ./analyzer_par_critical <arquivo_log>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "common.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Uso: %s <arquivo_log>\n", argv[0]);
        return 1;
    }

    // printf("[1/4] Carregando manifest...\n");
    HashTable *ht = ht_create(TABLE_SIZE);
    long n_urls = carregar_manifest(ht);
    // printf("      %ld URLs carregadas.\n", n_urls);

    // 2. le todo o log para a memoria
    printf("[2/4] Lendo log para memoria...\n");
    VetorLinhas *v = ler_log(argv[1]);
    printf("      %ld linhas lidas.\n", (*v).total);

    // 3. processa em paralelo
    int nthreads = omp_get_max_threads();
    printf("[3/4] Processando com %d threads (critical)...\n", nthreads);

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    long processadas = 0;

    #pragma omp parallel for schedule(dynamic, 1024) reduction(+:processadas)
    for (long i = 0; i < (*v).total; i++) {
        char url[MAX_URL];
        if (!extrair_url((*v).linhas[i], url)) {
            continue;
        }

        CacheNode *no = ht_get(ht, url);
        if (no) {
            // regiao critica global: so uma thread por vez
            #pragma omp critical
            {
                no->hit_count++; // (*no).hit_count++
            }
            processadas++;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec  - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) * 1000000000.0;

    printf("      %ld linhas processadas em %.3f s\n", processadas, tempo);

    liberar_vetor(v);

    // 4. salva resultado
    // printf("[4/4] Salvando %s...\n", RESULTS_FILE);
    ht_save_results(ht, RESULTS_FILE);

    ht_destroy(ht);
    // printf("Pronto!\n");
    return 0;
}
