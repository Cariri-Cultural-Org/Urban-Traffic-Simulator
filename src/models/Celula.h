#ifndef CELULA_H
#define CELULA_H

#include <pthread.h>

// Forward declaration de Veiculo.
struct Veiculo;

typedef struct
{
    int linha;
    int coluna;
    int ocupada;
    struct Veiculo *veiculo;
    pthread_mutex_t mutex;
} Celula;

void celula_init(Celula *c, int linha, int coluna);
void celula_destroy(Celula *c);
int celula_tentar_ocupar(Celula *c, struct Veiculo *v);
void celula_liberar(Celula *c);

#endif
