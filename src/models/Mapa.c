#include <stdlib.h>
#include "mapa_utils.c"

Mapa *mapa_criar(void)
{
    Mapa *m = calloc(1, sizeof(Mapa));
    if (!m) return NULL;

    m->linhas = MAPA_LINHAS;
    m->colunas = MAPA_COLUNAS;

    if (!alocar_celulas(m))
    {
        mapa_destruir(m);
        return NULL;
    }

    if (!alocar_vias(m))
    {
        mapa_destruir(m);
        return NULL;
    }

    if (!alocar_cruzamentos(m))
    {
        mapa_destruir(m);
        return NULL;
    }

    return m;
}

void mapa_destruir(Mapa *m)
{
    if (!m) return;
    if (m->cruzamentos) liberar_cruzamentos(m);
    if (m->vias) liberar_vias(m);
    if (m->celulas) liberar_celulas(m, m->linhas);
    free(m);
}

Celula *mapa_get_celula(const Mapa *m, int linha, int coluna)
{
    if (!mapa_posicao_valida(m, linha, coluna))
        return NULL;

    return &m->celulas[linha][coluna];
}

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

int mapa_posicao_valida(const Mapa *m, int linha, int coluna)
{
    if (!m)
        return 0;

    return linha >= 0 && linha < m->linhas &&
           coluna >= 0 && coluna < m->colunas;
}
