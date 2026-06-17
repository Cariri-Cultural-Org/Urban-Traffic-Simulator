#include "semaforo_transito.h"

void init_semaforo(SemaforoTransito* sem, int id, int t_verde, int t_vermelho) {
    sem->id = id;
    sem->estado = VERMELHO; // Inicia fechado por padrão
    sem->tempo_verde = t_verde;
    sem->tempo_vermelho = t_vermelho;
    sem->tick_atual = 0;
    
#ifdef _WIN32
    InitializeCriticalSection(&sem->mutex);
#else
    pthread_mutex_init(&sem->mutex, NULL);
#endif
}

void atualizar_semaforo(SemaforoTransito* sem) {
#ifdef _WIN32
    EnterCriticalSection(&sem->mutex);
#else
    pthread_mutex_lock(&sem->mutex);
#endif

    sem->tick_atual++;
    
    // Alterna o semáforo com base no tempo (ticks)
    if (sem->estado == VERMELHO && sem->tick_atual >= sem->tempo_vermelho) {
        sem->estado = VERDE;
        sem->tick_atual = 0; // Reseta o contador
    } else if (sem->estado == VERDE && sem->tick_atual >= sem->tempo_verde) {
        sem->estado = VERMELHO;
        sem->tick_atual = 0; // Reseta o contador
    }
    
#ifdef _WIN32
    LeaveCriticalSection(&sem->mutex);
#else
    pthread_mutex_unlock(&sem->mutex);
#endif
}

void destroy_semaforo(SemaforoTransito* sem) {
#ifdef _WIN32
    DeleteCriticalSection(&sem->mutex);
#else
    pthread_mutex_destroy(&sem->mutex);
#endif
}
