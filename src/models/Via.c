#include <stdlib.h>
#include "Via.h"

/* -----------------------------------------------------------------------
 * via_criar
 * Aloca e inicializa uma Via. O vetor de ponteiros para células é alocado
 * aqui (tamanho = num_celulas), mas preenchido pelo Mapa durante a
 * construção da malha.
 *
 * Retorna o ponteiro para a Via criada, ou NULL em caso de falha de
 * alocação.
 * ----------------------------------------------------------------------- */
Via *via_criar(int id, DirecaoVia direcao, SentidoVia sentido, int num_celulas)
{
    Via *v = malloc(sizeof(Via));
    if (!v)
        return NULL;

    v->id = id;
    v->direcao = direcao;
    v->sentido = sentido;
    v->num_celulas = num_celulas;

    /* Vetor de ponteiros — preenchido por mapa_criar() */
    v->celulas = calloc(num_celulas, sizeof(Celula *));
    if (!v->celulas)
    {
        free(v);
        return NULL;
    }

    /* Índices de cruzamentos — alocados por mapa_criar() após saber a qty */
    v->indices_cruzamentos = NULL;
    v->num_cruzamentos = 0;

    return v;
}

/* -----------------------------------------------------------------------
 * via_destruir
 * Libera a memória alocada pela via. Não destrói as células (que pertencem
 * ao Mapa e têm ciclo de vida próprio).
 * ----------------------------------------------------------------------- */
void via_destruir(Via *v)
{
    if (!v)
        return;

    free(v->indices_cruzamentos);
    free(v->celulas);
    free(v);
}

/* -----------------------------------------------------------------------
 * via_get_celula
 * Retorna o ponteiro para a célula na posição 'indice' desta via.
 * Retorna NULL se o índice estiver fora do intervalo [0, num_celulas).
 * ----------------------------------------------------------------------- */
Celula *via_get_celula(const Via *v, int indice)
{
    if (!v || indice < 0 || indice >= v->num_celulas)
        return NULL;
    return v->celulas[indice];
}

/* -----------------------------------------------------------------------
 * via_eh_cruzamento
 * Retorna 1 se a posição 'indice' na via coincide com um cruzamento,
 * 0 caso contrário.
 * Usado pela thread do veículo para saber se deve verificar o semáforo
 * antes de avançar.
 * ----------------------------------------------------------------------- */
int via_eh_cruzamento(const Via *v, int indice)
{
    if (!v || !v->indices_cruzamentos)
        return 0;

    for (int i = 0; i < v->num_cruzamentos; i++)
    {
        if (v->indices_cruzamentos[i] == indice)
            return 1;
    }

    return 0;
}
