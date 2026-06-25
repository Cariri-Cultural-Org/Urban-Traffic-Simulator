#include "Relogio_global.h"

int global_tick = 0;
os_mutex_t clock_mutex;
os_cond_t clock_cond;
bool simulation_running = true;

void init_relogio()
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
DWORD WINAPI thread_relogio(LPVOID arg)
{
#else
void *thread_relogio(void *arg)
{
#endif
#ifdef _WIN32
    (void)arg; // suppress warning
#endif
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

void esperar_proximo_tick(int tick_atual)
{
#ifdef _WIN32
    while (simulation_running)
    {
        EnterCriticalSection(&clock_mutex);
        if (global_tick != tick_atual)
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
    while (global_tick == tick_atual && simulation_running)
    {
        pthread_cond_wait(&clock_cond, &clock_mutex);
    }
    pthread_mutex_unlock(&clock_mutex);
#endif
}

void destroy_relogio()
{
#ifdef _WIN32
    DeleteCriticalSection(&clock_mutex);
    CloseHandle(clock_cond);
#else
    pthread_mutex_destroy(&clock_mutex);
    pthread_cond_destroy(&clock_cond);
#endif
}
