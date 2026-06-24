
#ifndef CRUZAMENTO_H
#define CRUZAMENTO_H
#include <pthread.h>
#include "Via.h"
#include "Semaforo.h"

typedef struct Cruzamento
{
    int id;
    int linha;
    int coluna;
    Via *via_h;
    Via *via_v;

    DirecaoVia direcao_verde;
    pthread_mutex_t mutex; // Bloqueia passagem durante o sinal verde
    pthread_cond_t cond_h;
    pthread_cond_t cond_v;

    // Suporte à ambulância
    int ambulancia_presente;
    DirecaoVia direcao_ambulancia;
} Cruzamento;

Cruzamento *cruzamento_init(int id, int linha, int coluna,
                     Via *via_h, Via *via_v);
void cruzamento_destroy(Cruzamento *c);
void cruzamento_alternar_sinal(Cruzamento *c);
void cruzamento_esperar_verde(Cruzamento *c, DirecaoVia via);

#endif
