#include <stdlib.h>
#include "Via.h"
#include "Cruzamento.h"

Via *via_criar(int id, DirecaoVia direcao, SentidoVia sentido, int num_celulas)
{
    Via *v = malloc(sizeof(Via));
    if (!v)
        return NULL;

    v->id = id;
    v->direcao = direcao;
    v->sentido = sentido;
    v->num_celulas = num_celulas;

    // inicializa vertor sem lixo de memória
    v->celulas = calloc(num_celulas, sizeof(Celula *));
    if (!v->celulas)
    {
        free(v);
        return NULL;
    }

    v->cruzamentos = calloc(num_celulas, sizeof(Cruzamento));
    if (!v->cruzamentos)
    {
        free(v->celulas); // também libera as células antes de destruir a via
        free(v);
        return NULL;
    }

    return v;
}

void via_destruir(Via *v)
{
    if (!v)
        return;

    free(v->cruzamentos);
    free(v->celulas);
    free(v);
}

Celula *via_get_celula(const Via *v, int indice)
{
    if (!v || indice < 0 || indice >= v->num_celulas)
        return NULL;
    return v->celulas[indice];
}

/*
- Retorna o `Cruzamento` se o indice na via coincide com um cruzamento, NULL caso contrário.
- Usado pela thread do veículo para saber se deve verificar o semáforo antes de avançar.
*/
Cruzamento *via_get_cruzamento(const Via *v, int indice)
{
    if (!v || indice < 0 || indice >= v->num_celulas)
        return NULL;

    return &v->cruzamentos[indice];
}
