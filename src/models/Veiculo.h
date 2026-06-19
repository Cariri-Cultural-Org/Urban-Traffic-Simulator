#ifndef VEICULO_H
#define VEICULO_H

#include <pthread.h>
#include "Via.h"

typedef struct Veiculo
{
    int id;
    int velocidade;       /* ticks por movimento: 1 = rápido, 2 = médio, 4 = lento */
    Via *via_atual;
    int index_celula_via; /* índice da célula atual em via_atual->celulas */
    int linha;
    int coluna;
    
    pthread_t thread;
    int ativo;            /* 1 = rodando, 0 = finalizado */
    void *mapa;           /* ponteiro para o Mapa */
} Veiculo;

/* Inicializa um veículo com os parâmetros básicos */
void veiculo_init(Veiculo *v, int id, int velocidade, Via *via, int index_celula_via, void *mapa);

#endif /* VEICULO_H */
