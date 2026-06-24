#ifndef SEMAFORO_H
#define SEMAFORO_H

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION os_mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t os_mutex_t;
#endif

typedef enum
{
    VERMELHO,
    VERDE
} EstadoSemaforo;

typedef struct
{
    int id;
    EstadoSemaforo estado;
    int tempo_verde;
    int tempo_vermelho;
    int tick_atual;
    os_mutex_t mutex;
} Semaforo;

// Inicializa o sem�foro de tr�nsito
void init_semaforo(Semaforo *sem, int id, int t_verde, int t_vermelho);

// Atualiza o estado do sem�foro a cada tick
void atualizar_semaforo(Semaforo *sem);

// Limpa recursos do sem�foro
void destroy_semaforo(Semaforo *sem);

#endif // SEMAFORO_H
