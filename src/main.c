#include <stdio.h>
#include <stdbool.h>
#include "models/GlobalClock.h"
#include "models/CityMap.h"

/* Inclusões de thread dependentes do SO */
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static DWORD WINAPI clock_test_thread(LPVOID arg)
#else
static void *clock_test_thread(void *arg)
#endif
{
    (void)arg;

    int observed_tick = global_tick;

    while (simulation_running)
    {
        wait_next_tick(observed_tick);

        if (!simulation_running)
            break;

        observed_tick = global_tick;
        printf("[Test] aguardou tick %d e seguiu vivo\n", observed_tick);
    }

    printf("[Test] encerrando junto com o relogio global\n");
    return 0;
}

int main(void)
{
    printf("--- Starting the Urban Traffic Simulator ---\n");

    /* ── M0-C: Criar o CityMap real ─────────────────────────────────────── */
    CityMap *city_map = city_map_create();
    if (!city_map)
    {
        fprintf(stderr, "ERRO: Falha ao criar o CityMap.\n");
        return 1;
    }

    printf("Mapa criado: %d linhas x %d colunas\n",
           city_map->rows, city_map->columns);
    printf("Vias: %d  |  Cruzamentos: %d\n",
           city_map->road_count, city_map->intersection_count);

    /* ── Inicializa o relógio global ────────────────────────────────────── */
    init_global_clock();

    /* Cria a thread do relógio */
    os_thread_t clock_thread;
#ifdef _WIN32
    clock_thread = CreateThread(NULL, 0, thread_global_clock, NULL, 0, NULL);
#else
    pthread_create(&clock_thread, NULL, thread_global_clock, NULL);
#endif

    /* Thread de teste para validar espera e encerramento limpo */
    os_thread_t test_thread;
#ifdef _WIN32
    test_thread = CreateThread(NULL, 0, clock_test_thread, NULL, 0, NULL);
#else
    pthread_create(&test_thread, NULL, clock_test_thread, NULL);
#endif

    /* ── Loop de simulação (5 ticks de demonstração) ────────────────────── */
    for (int i = 1; i <= 5; i++)
    {
        wait_next_tick(global_tick);
        printf("[Tick: %02d] Simulador ativo — mapa %dx%d, %d cruzamentos\n",
               global_tick,
               city_map->rows, city_map->columns,
               city_map->intersection_count);
    }

    printf("\nEncerrando simulação...\n");

    /* ── Finaliza o relógio ─────────────────────────────────────────────── */
    stop_global_clock();

#ifdef _WIN32
    WaitForSingleObject(test_thread, INFINITE);
    CloseHandle(test_thread);
    WaitForSingleObject(clock_thread, INFINITE);
    CloseHandle(clock_thread);
#else
    pthread_join(test_thread, NULL);
    pthread_join(clock_thread, NULL);
#endif

    destroy_global_clock();

    /* ── M0-A / M0-C: Destruir o CityMap sem invalid free ──────────────── */
    city_map_destroy(city_map);
    city_map = NULL;

    printf("Cleanup concluído. Saindo do simulador.\n");
    return 0;
}
