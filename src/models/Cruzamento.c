#include <stdlib.h>
#include "Cruzamento.h"
#include "Via.h"

Cruzamento *cruzamento_init(int id, int linha, int coluna, Via *via_h, Via *via_v)
{
    Cruzamento *c = malloc(sizeof(Cruzamento));
    c->id = id;
    c->linha = linha;
    c->coluna = coluna;
    c->via_h = via_h;
    c->via_v = via_v;
    c->direcao_verde = VIA_HORIZONTAL;

    c->ambulancia_presente = 0; // Ambulância inativa por padrão
    c->direcao_ambulancia = VIA_HORIZONTAL;

    pthread_mutex_init(&c->mutex, NULL);
    pthread_cond_init(&c->cond_h, NULL);
    pthread_cond_init(&c->cond_v, NULL);

    return c;
}

void cruzamento_destroy(Cruzamento *c)
{
    pthread_cond_destroy(&c->cond_v);
    pthread_cond_destroy(&c->cond_h);
    pthread_mutex_destroy(&c->mutex);
}

void cruzamento_alternar_sinal(Cruzamento *c)
{
    pthread_mutex_lock(&c->mutex);

    /*
     * Se a ambulância está presente, verifica se a sua via já está verde.
     * Em caso positivo, não alterna — ela tem prioridade.
     */
    if (c->ambulancia_presente)
    {
        if (c->direcao_ambulancia == VIA_HORIZONTAL && c->direcao_verde == VIA_HORIZONTAL)
        {
            pthread_mutex_unlock(&c->mutex);
            return;
        }
        if (c->direcao_ambulancia == VIA_VERTICAL && c->direcao_verde == VIA_VERTICAL)
        {
            pthread_mutex_unlock(&c->mutex);
            return;
        }
    }

    if (c->direcao_verde == VIA_HORIZONTAL)
    {
        c->direcao_verde = VIA_VERTICAL;
        pthread_cond_broadcast(&c->cond_v); /* acorda carros da via vertical */
    }
    else
    {
        c->direcao_verde = VIA_HORIZONTAL;
        pthread_cond_broadcast(&c->cond_h); /* acorda carros da via horizontal */
    }

    pthread_mutex_unlock(&c->mutex);
}


void cruzamento_esperar_verde(Cruzamento *c, DirecaoVia via)
{
    if (via == VIA_HORIZONTAL)
    {
        while (c->direcao_verde != VIA_HORIZONTAL) pthread_cond_wait(&c->cond_h, &c->mutex);
    }
    else
    {
        while (c->direcao_verde != VIA_VERTICAL) pthread_cond_wait(&c->cond_v, &c->mutex);
    }
}
