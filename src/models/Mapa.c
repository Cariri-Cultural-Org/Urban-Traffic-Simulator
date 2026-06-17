#include <stdlib.h>
#include <stdio.h>
#include "Mapa.h"

/* =======================================================================
 * Layout fixo da malha viária
 *
 * O mapa tem dimensões MAPA_LINHAS x MAPA_COLUNAS (definidas em Mapa.h).
 * As ruas são definidas abaixo como linhas (horizontais) e colunas
 * (verticais) fixas. Cada interseção gera um Cruzamento.
 *
 * Esquema visual (20 linhas x 40 colunas):
 *
 *          col8   col16  col24  col32
 *           |      |      |      |
 * lin4  ----+------+------+------+----  (mão única →)
 *           |      |      |      |
 * lin10 ----+------+------+------+----  (mão dupla)
 *           |      |      |      |
 * lin16 ----+------+------+------+----  (mão dupla)
 *           |      |      |      |
 *         dupla  única  dupla  dupla    (sentidos das vias verticais)
 *
 * Total: 3 vias horizontais × 4 vias verticais = 12 cruzamentos (≥ 8 ✓)
 * Mão única obrigatória: linha 4 (→) e coluna 16 (↓)              ✓
 * ======================================================================= */

/* Linhas onde existem vias horizontais e seus sentidos */
static const int LINHAS_H[] = {4, 10, 16};
static const int SENTIDOS_H[] = {VIA_MAO_UNICA, VIA_MAO_DUPLA, VIA_MAO_DUPLA};
#define NUM_VIAS_H 3

/* Colunas onde existem vias verticais e seus sentidos */
static const int COLUNAS_V[] = {8, 16, 24, 32};
static const int SENTIDOS_V[] = {VIA_MAO_DUPLA, VIA_MAO_UNICA, VIA_MAO_DUPLA, VIA_MAO_DUPLA};
#define NUM_VIAS_V 4

#define NUM_VIAS_TOTAL (NUM_VIAS_H + NUM_VIAS_V)
#define NUM_CRUZAMENTOS (NUM_VIAS_H * NUM_VIAS_V)

/* -----------------------------------------------------------------------
 * Funções internas de cleanup usadas em caso de falha de alocação
 * ----------------------------------------------------------------------- */
static void liberar_celulas(Mapa *m, int linhas_alocadas)
{
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

/* -----------------------------------------------------------------------
 * mapa_criar
 * Constrói a malha viária completa:
 *   1. Aloca e inicializa a matriz de Celulas
 *   2. Cria as Vias horizontais e verticais, associando ponteiros de célula
 *   3. Cria os Cruzamentos em cada interseção, conectando as Vias
 *
 * Retorna o Mapa pronto para uso, ou NULL em qualquer falha de alocação.
 * ----------------------------------------------------------------------- */
Mapa *mapa_criar(void)
{
    Mapa *m = calloc(1, sizeof(Mapa)); /* calloc zera todos os ponteiros */
    if (!m)
        return NULL;

    m->linhas = MAPA_LINHAS;
    m->colunas = MAPA_COLUNAS;

    /* ------------------------------------------------------------------ */
    /* 1. Alocar matriz de células                                         */
    /* ------------------------------------------------------------------ */
    m->celulas = malloc(m->linhas * sizeof(Celula *));
    if (!m->celulas)
    {
        free(m);
        return NULL;
    }

    for (int l = 0; l < m->linhas; l++)
    {
        m->celulas[l] = malloc(m->colunas * sizeof(Celula));
        if (!m->celulas[l])
        {
            liberar_celulas(m, l);
            free(m);
            return NULL;
        }
        for (int c = 0; c < m->colunas; c++)
        {
            celula_init(&m->celulas[l][c], l, c);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 2. Criar vias                                                       */
    /* ------------------------------------------------------------------ */
    m->num_vias = NUM_VIAS_TOTAL;
    m->vias = calloc(m->num_vias, sizeof(Via *));
    if (!m->vias)
    {
        mapa_destruir(m);
        return NULL;
    }

    int via_id = 0;

    /* --- Vias horizontais --- */
    for (int i = 0; i < NUM_VIAS_H; i++)
    {
        int row = LINHAS_H[i];

        Via *v = via_criar(via_id++, VIA_HORIZONTAL, SENTIDOS_H[i], m->colunas);
        if (!v)
        {
            mapa_destruir(m);
            return NULL;
        }

        /* Aponta cada posição da via para a célula correspondente na matriz */
        for (int c = 0; c < m->colunas; c++)
        {
            v->celulas[c] = &m->celulas[row][c];
        }

        /* Registra quais índices (colunas) são cruzamentos nesta via */
        v->num_cruzamentos = NUM_VIAS_V;
        v->indices_cruzamentos = malloc(NUM_VIAS_V * sizeof(int));
        if (!v->indices_cruzamentos)
        {
            via_destruir(v);
            mapa_destruir(m);
            return NULL;
        }
        for (int j = 0; j < NUM_VIAS_V; j++)
        {
            v->indices_cruzamentos[j] = COLUNAS_V[j];
        }

        m->vias[i] = v;
    }

    /* --- Vias verticais --- */
    for (int i = 0; i < NUM_VIAS_V; i++)
    {
        int col = COLUNAS_V[i];

        Via *v = via_criar(via_id++, VIA_VERTICAL, SENTIDOS_V[i], m->linhas);
        if (!v)
        {
            mapa_destruir(m);
            return NULL;
        }

        /* Aponta cada posição da via para a célula correspondente na matriz */
        for (int l = 0; l < m->linhas; l++)
        {
            v->celulas[l] = &m->celulas[l][col];
        }

        /* Registra quais índices (linhas) são cruzamentos nesta via */
        v->num_cruzamentos = NUM_VIAS_H;
        v->indices_cruzamentos = malloc(NUM_VIAS_H * sizeof(int));
        if (!v->indices_cruzamentos)
        {
            via_destruir(v);
            mapa_destruir(m);
            return NULL;
        }
        for (int j = 0; j < NUM_VIAS_H; j++)
        {
            v->indices_cruzamentos[j] = LINHAS_H[j];
        }

        m->vias[NUM_VIAS_H + i] = v;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Criar cruzamentos em cada interseção horizontal × vertical       */
    /* ------------------------------------------------------------------ */
    m->num_cruzamentos = NUM_CRUZAMENTOS;
    m->cruzamentos = calloc(m->num_cruzamentos, sizeof(Cruzamento *));
    if (!m->cruzamentos)
    {
        mapa_destruir(m);
        return NULL;
    }

    int cid = 0;
    for (int i = 0; i < NUM_VIAS_H; i++)
    {
        for (int j = 0; j < NUM_VIAS_V; j++)
        {
            Cruzamento *c = malloc(sizeof(Cruzamento));
            if (!c)
            {
                mapa_destruir(m);
                return NULL;
            }

            cruzamento_init(c, cid,
                            LINHAS_H[i],            /* linha  do cruzamento */
                            COLUNAS_V[j],           /* coluna do cruzamento */
                            m->vias[i],             /* via horizontal       */
                            m->vias[NUM_VIAS_H + j] /* via vertical         */
            );

            m->cruzamentos[cid] = c;
            cid++;
        }
    }

    return m;
}

/* -----------------------------------------------------------------------
 * mapa_destruir
 * Libera toda a memória do mapa na ordem inversa da criação:
 *   cruzamentos → vias → células → struct Mapa
 * Seguro chamar com um Mapa parcialmente inicializado (usa checagem de NULL).
 * ----------------------------------------------------------------------- */
void mapa_destruir(Mapa *m)
{
    if (!m)
        return;

    /* Cruzamentos */
    if (m->cruzamentos)
    {
        for (int i = 0; i < m->num_cruzamentos; i++)
        {
            if (m->cruzamentos[i])
            {
                cruzamento_destroy(m->cruzamentos[i]);
                free(m->cruzamentos[i]);
            }
        }
        free(m->cruzamentos);
    }

    /* Vias */
    if (m->vias)
    {
        for (int i = 0; i < m->num_vias; i++)
        {
            via_destruir(m->vias[i]);
        }
        free(m->vias);
    }

    /* Células */
    if (m->celulas)
    {
        liberar_celulas(m, m->linhas);
    }

    free(m);
}

/* -----------------------------------------------------------------------
 * mapa_get_celula
 * Retorna ponteiro para a célula em (linha, coluna), ou NULL se inválido.
 * ----------------------------------------------------------------------- */
Celula *mapa_get_celula(const Mapa *m, int linha, int coluna)
{
    if (!mapa_posicao_valida(m, linha, coluna))
        return NULL;
    return &m->celulas[linha][coluna];
}

/* -----------------------------------------------------------------------
 * mapa_get_cruzamento
 * Percorre a lista de cruzamentos e retorna o que está em (linha, coluna),
 * ou NULL se não houver cruzamento nessa posição.
 * ----------------------------------------------------------------------- */
Cruzamento *mapa_get_cruzamento(const Mapa *m, int linha, int coluna)
{
    if (!m || !m->cruzamentos)
        return NULL;

    for (int i = 0; i < m->num_cruzamentos; i++)
    {
        Cruzamento *c = m->cruzamentos[i];
        if (c && c->linha == linha && c->coluna == coluna)
            return c;
    }

    return NULL;
}

/* -----------------------------------------------------------------------
 * mapa_posicao_valida
 * Retorna 1 se (linha, coluna) está dentro dos limites do mapa, 0 se não.
 * ----------------------------------------------------------------------- */
int mapa_posicao_valida(const Mapa *m, int linha, int coluna)
{
    if (!m)
        return 0;
    return (linha >= 0 && linha < m->linhas &&
            coluna >= 0 && coluna < m->colunas);
}
