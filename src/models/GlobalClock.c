#include "GlobalClock.h"

int global_tick = 0;
os_mutex_t clock_mutex;
os_cond_t clock_cond;
bool simulation_running = true;

void init_global_clock(void)
{
    global_tick = 0;
    simulation_running = true;
#ifdef _WIN32
    InitializeCriticalSection(&clock_mutex);
    clock_cond = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&clock_mutex, NULL);
    pthread_cond_init(&clock_cond, NULL); // Inicializa a variavel de condicao POSIX
#endif
}

#ifdef _WIN32
DWORD WINAPI thread_global_clock(LPVOID arg)
{
#else
void *thread_global_clock(void *arg)
{
#endif
    (void)arg; // suppress warning

    while (simulation_running)
    {
        // Pausa simulando a passagem de tempo de 1 tick (ex: 100ms)
#ifdef _WIN32
        Sleep(100);
        EnterCriticalSection(&clock_mutex);
#else
        usleep(100000);
        pthread_mutex_lock(&clock_mutex);
#endif

        global_tick++; // Atualiza o tick

#ifdef _WIN32
        LeaveCriticalSection(&clock_mutex);
        PulseEvent(clock_cond); // Dispara evento simulando Broadcast
#else
        pthread_cond_broadcast(&clock_cond); // Acorda TODOS usando var de condicao!
        pthread_mutex_unlock(&clock_mutex);
#endif
    }
    return 0;
}

void wait_next_tick(int current_tick)
{
#ifdef _WIN32
    while (simulation_running)
    {
        EnterCriticalSection(&clock_mutex);
        if (global_tick != current_tick)
        {
            LeaveCriticalSection(&clock_mutex);
            break;
        }
        LeaveCriticalSection(&clock_mutex);
        WaitForSingleObject(clock_cond, INFINITE); // Aguarda o Evento no Windows
    }
#else
    pthread_mutex_lock(&clock_mutex);
    // Variavel de condicao: dorme sem gastar CPU ate chegar o Broadcast do relogio
    while (global_tick == current_tick && simulation_running)
    {
        pthread_cond_wait(&clock_cond, &clock_mutex);
    }
    pthread_mutex_unlock(&clock_mutex);
#endif
}

void stop_global_clock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&clock_mutex);
    simulation_running = false;
    LeaveCriticalSection(&clock_mutex);
    SetEvent(clock_cond);
#else
    pthread_mutex_lock(&clock_mutex);
    simulation_running = false;
    pthread_cond_broadcast(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);
#endif
}

void destroy_global_clock(void)
{
#ifdef _WIN32
    DeleteCriticalSection(&clock_mutex);
    CloseHandle(clock_cond);
#else
    pthread_mutex_destroy(&clock_mutex);
    pthread_cond_destroy(&clock_cond);
#endif
}
