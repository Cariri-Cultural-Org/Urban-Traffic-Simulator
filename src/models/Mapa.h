#ifndef MAPA_H
#define MAPA_H

#include "Celula.h"
#include "Via.h"
#include "Cruzamento.h"

#define MAPA_LINHAS 20
#define MAPA_COLUNAS 40

#define MAPA_MIN_CRUZAMENTOS 8

typedef struct
{
    int linhas;
    int colunas;

    Celula **celulas;
    Via **vias;
    int num_vias;

    Cruzamento **cruzamentos;
    int num_cruzamentos;
} Mapa;

Mapa *mapa_criar(void);
void mapa_destruir(Mapa *m);
Celula *mapa_get_celula(const Mapa *m, int linha, int coluna);
Cruzamento *mapa_get_cruzamento(const Mapa *m, int linha, int coluna);
int mapa_posicao_valida(const Mapa *m, int linha, int coluna);

#endif
