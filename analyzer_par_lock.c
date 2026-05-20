/*
 * analyzer_par_lock.c
 * Versao paralela com lock por bucket (omp_lock_t).
 *
 * Em vez de uma trava global como no critical, cada bucket da tabela
 * hash tem seu proprio lock. Threads so se bloqueiam se tentarem
 * acessar o mesmo bucket ao mesmo tempo.
 *
 * Fluxo por linha:
 *   1. extrai a URL
 *   2. calcula qual bucket e o dela
 *   3. trava aquele bucket
 *   4. incrementa o contador
 *   5. destrava
 *
 * Uso: ./analyzer_par_lock <arquivo_log>
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

    // 1. carrega o manifest
    // printf("[1/5] Carregando manifest...\n");
    HashTable *ht = ht_create(TABLE_SIZE);
    long n_urls = carregar_manifest(ht);
    // printf("      %ld URLs carregadas.\n", n_urls);

    // 2. cria um lock para cada bucket da tabela
    omp_lock_t *locks = (omp_lock_t *)malloc(sizeof(omp_lock_t) * TABLE_SIZE);
    if (!locks) { 
        perror("malloc locks"); 
        return 1; }

    for (size_t i = 0; i < TABLE_SIZE; i++)
        omp_init_lock(&locks[i]);

    printf("[2/5] %d locks criados (um por bucket).\n", TABLE_SIZE);

    // 3. le todo o log para a memoria
    printf("[3/5] Lendo log para memoria...\n");
    VetorLinhas *v = ler_log(argv[1]);
    printf("      %ld linhas lidas.\n", (*v).total);

    // 4. processa em paralelo
    int nthreads = omp_get_max_threads();
    printf("[4/5] Processando com %d threads (bucket lock)...\n", nthreads);

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    long processadas = 0;

    #pragma omp parallel for schedule(dynamic, 1024) reduction(+:processadas)
    for (long i = 0; i < (*v).total; i++) {
        char url[MAX_URL];
        if (!extrair_url((*v).linhas[i], url)) {
            continue;
        }

        // calcula o bucket antes de travar
        size_t bucket = ht_bucket_of(ht, url);

        omp_set_lock(&locks[bucket]);

        CacheNode *no = ht_get(ht, url);
        if (no) {
            no->hit_count++; // (*no).hit_count++
            processadas++;
        }

        omp_unset_lock(&locks[bucket]);
    }

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec  - inicio.tv_sec) + (fim.tv_nsec - inicio.tv_nsec) * 1000000000.0;

    printf("      %ld linhas processadas em %.3f s\n", processadas, tempo);

    liberar_vetor(v);

    // destroi os locks
    for (size_t i = 0; i < TABLE_SIZE; i++)
        omp_destroy_lock(&locks[i]);
    free(locks);

    // 5. salva resultado
    // printf("[5/5] Salvando %s...\n", RESULTS_FILE);
    ht_save_results(ht, RESULTS_FILE);

    ht_destroy(ht);
    // printf("Pronto!\n");
    return 0;
}