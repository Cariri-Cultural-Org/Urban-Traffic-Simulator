#ifndef SEMAFORO_TRANSITO_H
#define SEMAFORO_TRANSITO_H

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION os_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t os_mutex_t;
#endif

typedef enum {
    VERMELHO,
    VERDE
} EstadoSemaforo;

typedef struct {
    int id;
    EstadoSemaforo estado;
    int tempo_verde;
    int tempo_vermelho;
    int tick_atual;
    os_mutex_t mutex;
} SemaforoTransito;

// Inicializa o semáforo de trânsito
void init_semaforo(SemaforoTransito* sem, int id, int t_verde, int t_vermelho);

// Atualiza o estado do semáforo a cada tick
void atualizar_semaforo(SemaforoTransito* sem);

// Limpa recursos do semáforo
void destroy_semaforo(SemaforoTransito* sem);

#endif // SEMAFORO_TRANSITO_H
