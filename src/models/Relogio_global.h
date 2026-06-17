#ifndef RELOGIO_GLOBAL_H
#define RELOGIO_GLOBAL_H

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
typedef CRITICAL_SECTION os_mutex_t;
typedef HANDLE os_thread_t;
typedef HANDLE os_cond_t; // Usado para driblar o GCC antigo do Windows
#else
#include <pthread.h>
#include <unistd.h>
typedef pthread_mutex_t os_mutex_t;
typedef pthread_t os_thread_t;
typedef pthread_cond_t os_cond_t; // <-- A Variavel de Condicao para Linux/SO!
#endif

// Variaveis globais para o relogio e controle de concorrencia
extern int global_tick;
extern os_mutex_t clock_mutex;
extern os_cond_t clock_cond;
extern bool simulation_running;

// Inicializa as estruturas do relogio
void init_relogio();

// Funcao da thread que ira gerenciar o relogio global
#ifdef _WIN32
DWORD WINAPI thread_relogio(LPVOID arg);
#else
void* thread_relogio(void* arg);
#endif

// Funcao para uma thread aguardar ate o proximo tick
void esperar_proximo_tick(int tick_atual);

// Limpa recursos do relogio
void destroy_relogio();

#endif // RELOGIO_GLOBAL_H
