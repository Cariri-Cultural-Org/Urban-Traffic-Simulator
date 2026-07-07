#ifndef GLOBAL_CLOCK_H
#define GLOBAL_CLOCK_H

#include <stdbool.h>

#include "pthread_compat.h"

#ifndef _WIN32
#include <unistd.h>
#endif

typedef pthread_mutex_t os_mutex_t;
typedef pthread_t os_thread_t;
typedef pthread_cond_t os_cond_t;

// Variaveis globais para o relogio e controle de concorrencia
extern int global_tick;
extern os_mutex_t clock_mutex;
extern os_cond_t clock_cond;
extern bool simulation_running;

// Inicializa as estruturas do relogio
void init_global_clock(void);

// Funcao da thread que ira gerenciar o relogio global
void *thread_global_clock(void *arg);

// Funcao para uma thread aguardar ate o proximo tick
void wait_next_tick(int current_tick);

// Solicita encerramento e acorda qualquer thread bloqueada no relogio
void stop_global_clock(void);

// Limpa recursos do relogio
void destroy_global_clock(void);

#endif // GLOBAL_CLOCK_H
