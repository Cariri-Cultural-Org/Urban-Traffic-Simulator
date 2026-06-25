#include <stdlib.h>
#include "Celula.h"

Celula *celula_init(int linha, int coluna)
{
    Celula *c = malloc(sizeof(Celula));
    if (c == NULL)
        return NULL;
    c->linha = linha;
    c->coluna = coluna;
    c->ocupada = 0;
    c->veiculo = NULL;
    pthread_mutex_init(&c->mutex, NULL);
    return c;
}

void celula_destroy(Celula *c)
{
    pthread_mutex_destroy(&c->mutex);
    free(c);
}

int celula_tentar_ocupar(Celula *c, struct Veiculo *v)
{
    int sucesso = 0;

    pthread_mutex_lock(&c->mutex);

    if (!c->ocupada)
    {
        c->ocupada = 1;
        c->veiculo = v;
        sucesso = 1;
    }

    pthread_mutex_unlock(&c->mutex);

    return sucesso;
}

void celula_liberar(Celula *c)
{
    pthread_mutex_lock(&c->mutex);

    c->ocupada = 0;
    c->veiculo = NULL;

    pthread_mutex_unlock(&c->mutex);
}
