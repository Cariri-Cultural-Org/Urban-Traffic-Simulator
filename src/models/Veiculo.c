#include "Veiculo.h"

void veiculo_init(Veiculo *v, int id, int velocidade, Via *via, int index_celula_via, void *mapa)
{
    v->id = id;
    v->velocidade = velocidade;
    v->via_atual = via;
    v->index_celula_via = index_celula_via;
    v->linha = 0;
    v->coluna = 0;
    v->ativo = 1;
    v->mapa = mapa;
}
