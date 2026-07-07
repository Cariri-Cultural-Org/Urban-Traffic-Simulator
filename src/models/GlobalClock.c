#include "GlobalClock.h"

int global_tick = 0;
os_mutex_t clock_mutex;
os_cond_t clock_cond;
bool simulation_running = true;

void init_global_clock(void)
{
    global_tick = 0;
    simulation_running = true;
    pthread_mutex_init(&clock_mutex, NULL);
    pthread_cond_init(&clock_cond, NULL); // Inicializa a variavel de condicao POSIX
}

void *thread_global_clock(void *arg)
{
    (void)arg; // suppress warning

    while (simulation_running)
    {
        // Pausa simulando a passagem de tempo de 1 tick (ex: 100ms)
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif

        pthread_mutex_lock(&clock_mutex);
        global_tick++; // Atualiza o tick

        pthread_cond_broadcast(&clock_cond); // Acorda TODOS usando var de condicao!
        pthread_mutex_unlock(&clock_mutex);
    }
    return NULL;
}

void wait_next_tick(int current_tick)
{
    pthread_mutex_lock(&clock_mutex);
    // Variavel de condicao: dorme sem gastar CPU ate chegar o Broadcast do relogio
    while (global_tick == current_tick && simulation_running)
    {
        pthread_cond_wait(&clock_cond, &clock_mutex);
    }
    pthread_mutex_unlock(&clock_mutex);
}

void stop_global_clock(void)
{
    pthread_mutex_lock(&clock_mutex);
    simulation_running = false;
    pthread_cond_broadcast(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);
}

void destroy_global_clock(void)
{
    pthread_mutex_destroy(&clock_mutex);
    pthread_cond_destroy(&clock_cond);
}
