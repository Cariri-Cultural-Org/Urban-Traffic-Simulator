#include <stdlib.h>

#include "Cruzamento.h"
#include "Mapa.h"
#include "Via.h"

/* Linhas onde existem vias horizontais e seus sentidos */
static const int LINHAS_H[] = {4, 10, 16};

static const SentidoVia SENTIDOS_H[] = {
    VIA_MAO_UNICA,
    VIA_MAO_DUPLA,
    VIA_MAO_DUPLA
};

#define NUM_VIAS_H 3

/* Colunas onde existem vias verticais e seus sentidos */
static const int COLUNAS_V[] = {8, 16, 24, 32};

static const SentidoVia SENTIDOS_V[] = {
    VIA_MAO_DUPLA,
    VIA_MAO_UNICA,
    VIA_MAO_DUPLA,
    VIA_MAO_DUPLA
};

#define NUM_VIAS_V 4

#define NUM_VIAS_TOTAL (NUM_VIAS_H + NUM_VIAS_V)
#define NUM_CRUZAMENTOS (NUM_VIAS_H * NUM_VIAS_V)

static void liberar_cruzamentos(Mapa *m)
{
    if (!m || !m->cruzamentos) return;

    for (int i = 0; i < m->num_cruzamentos; i++)
    {
        if (m->cruzamentos[i])
        {
            cruzamento_destroy(m->cruzamentos[i]);
        }
    }

    free(m->cruzamentos);
    m->cruzamentos = NULL;
}
static void liberar_celulas(Mapa *m, int linhas_alocadas)
{
    if (!m || !m->celulas) return;

    for (int l = 0; l < linhas_alocadas; l++)
    {
        if (m->celulas[l])
        {
            for (int c = 0; c < m->colunas; c++)
            {
                celula_destroy(&m->celulas[l][c]);
            }

            free(m->celulas[l]);
        }
    }

    free(m->celulas);
    m->celulas = NULL;
}
static void liberar_vias(Mapa *m)
{
    if (!m || !m->vias) return;

    for (int i = 0; i < m->num_vias; i++)
    {
        via_destruir(m->vias[i]);
    }

    free(m->vias);
    m->vias = NULL;
}

static int alocar_celulas(Mapa *m)
{
    if (!m) return 0;

    m->celulas = malloc(m->linhas * sizeof(Celula *));
    if (!m->celulas) return 0;

    for (int l = 0; l < m->linhas; l++)
    {
        m->celulas[l] = malloc(m->colunas * sizeof(Celula));
        if (!m->celulas[l])
        {
            liberar_celulas(m, l);
            return 0;
        }

        for (int c = 0; c < m->colunas; c++)
        {
            celula_init(&m->celulas[l][c], l, c);
        }
    }

    return 1;
}

static int mapa_init_vias(Mapa *m, DirecaoVia d)
{
    if (!m || !m->vias || !m->celulas) return 0;

    int num_vias = (d == VIA_HORIZONTAL) ? NUM_VIAS_H : NUM_VIAS_V;
    int num_celulas = (d == VIA_HORIZONTAL) ? m->colunas : m->linhas;

    const int *posicoes = (d == VIA_HORIZONTAL) ? LINHAS_H : COLUNAS_V;
    const SentidoVia *sentidos = (d == VIA_HORIZONTAL) ? SENTIDOS_H : SENTIDOS_V;

    int offset = (d == VIA_HORIZONTAL) ? 0 : NUM_VIAS_H;

    for (int idx = 0; idx < num_vias; idx++)
    {
        m->vias[offset + idx] = via_criar(
            offset + idx,
            d,
            sentidos[idx],
            num_celulas
        );

        if (!m->vias[offset + idx]) return 0;

        int posicao = posicoes[idx];

        for (int cell = 0; cell < num_celulas; cell++)
        {
            int linha = (d == VIA_HORIZONTAL) ? posicao : cell;
            int coluna = (d == VIA_HORIZONTAL) ? cell : posicao;

            m->vias[offset + idx]->celulas[cell] = &m->celulas[linha][coluna];
        }
    }

    return 1;
}
static int alocar_vias(Mapa *m)
{
    if (!m) return 0;

    m->num_vias = NUM_VIAS_TOTAL;
    m->vias = calloc(m->num_vias, sizeof(Via *));

    if (!m->vias) return 0;

    if (!mapa_init_vias(m, VIA_HORIZONTAL))
        return 0;
    if (!mapa_init_vias(m, VIA_VERTICAL))
        return 0;

    return 1;
}

static int alocar_cruzamentos(Mapa *m)
{
    if (!m) return 0;

    m->num_cruzamentos = NUM_CRUZAMENTOS;
    m->cruzamentos = calloc(m->num_cruzamentos, sizeof(Cruzamento *));

    if (!m->cruzamentos) return 0;

    int cid = 0;

    for (int i = 0; i < NUM_VIAS_H; i++)
    {
        for (int j = 0; j < NUM_VIAS_V; j++)
        {
            Cruzamento *c = cruzamento_init(
                cid,
                LINHAS_H[i],
                COLUNAS_V[j],
                m->vias[i],
                m->vias[NUM_VIAS_H + j]
            );

            if (!c) return 0;


            m->cruzamentos[cid] = c;
            cid++;
        }
    }
    return 1;
}
