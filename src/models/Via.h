
#ifndef VIA_H
#define VIA_H

#include "Celula.h"

/*
 * Direção da via no mapa.
 */
typedef enum
{
    VIA_HORIZONTAL = 0,
    VIA_VERTICAL = 1
} DirecaoVia;

/*
 * Sentido de tráfego da via.
 * O projeto exige ao menos uma via de mão única (VIA_MAO_UNICA).
 */
typedef enum
{
    VIA_MAO_UNICA = 0, /* tráfego apenas num sentido           */
    VIA_MAO_DUPLA = 1  /* tráfego nos dois sentidos (2 faixas) */
} SentidoVia;

/*
 * Via representa uma rua completa da malha urbana — uma sequência contínua
 * de células, podendo ser horizontal ou vertical, de mão única ou dupla.
 *
 * Numa via de mão dupla, 'celulas' guarda UMA faixa. A faixa oposta é
 * representada por outra instância de Via com a mesma posição mas sentido
 * invertido (o Mapa mantém o par).
 */
typedef struct
{
    int id;             /* identificador único da via                  */
    DirecaoVia direcao; /* HORIZONTAL ou VERTICAL                      */
    SentidoVia sentido; /* MAO_UNICA ou MAO_DUPLA                      */

    /*
     * Vetor de ponteiros para as células que compõem esta via,
     * em ordem do sentido de tráfego (início → fim).
     */
    Celula **celulas;
    int num_celulas; /* quantidade de células nesta via             */

    /*
     * Índices dos cruzamentos ao longo desta via.
     * Cada valor é a posição dentro de 'celulas' onde há um cruzamento.
     * Útil para que um veículo saiba quando está chegando a um cruzamento.
     */
    int *indices_cruzamentos;
    int num_cruzamentos;

} Via;

/* ---------- Funções auxiliares (implementadas em Via.c) ---------- */

/* Aloca e inicializa uma Via. Retorna NULL em caso de falha. */
Via *via_criar(int id, DirecaoVia direcao, SentidoVia sentido,
               int num_celulas);

/* Libera toda a memória associada à via (não destrói as células). */
void via_destruir(Via *v);

/*
 * Retorna o ponteiro para a célula na posição 'indice' da via.
 * Retorna NULL se o índice estiver fora do intervalo.
 */
Celula *via_get_celula(const Via *v, int indice);

/*
 * Retorna 1 se a célula de índice 'indice' é um cruzamento, 0 caso contrário.
 */
int via_eh_cruzamento(const Via *v, int indice);

#endif /* VIA_H */
