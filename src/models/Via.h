#ifndef VIA_H
#define VIA_H
#include "Celula.h"

typedef struct Cruzamento Cruzamento ;

typedef enum
{
    VIA_HORIZONTAL,
    VIA_VERTICAL,
} DirecaoVia;

typedef enum
{
    VIA_MAO_UNICA = 0,
    VIA_MAO_DUPLA = 1
    /* Numa via de mão dupla, 'celulas' guarda UMA faixa. A faixa oposta é
     * representada por outra instância de Via com a mesma posição mas sentido
     * invertido (o Mapa mantém o par). */
} SentidoVia;

typedef struct
{
    int id;
    DirecaoVia direcao;
    SentidoVia sentido;

    Celula **celulas; // matriz de células
    Cruzamento *cruzamentos; // vetor de cruzamentos
    int num_celulas;
} Via;

Via *via_criar(int id, DirecaoVia direcao, SentidoVia sentido, int num_celulas);
void via_destruir(Via *v);
Celula *via_get_celula(const Via *v, int indice);
Cruzamento *via_get_cruzamento(const Via *v, int indice);

#endif
