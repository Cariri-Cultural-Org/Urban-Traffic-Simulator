#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "Ambulancia.h"
#include "Relogio_global.h"
#include "Mapa.h"
#include "Cruzamento.h"

#ifdef _WIN32
DWORD WINAPI thread_ambulancia(LPVOID arg)
#else
void *thread_ambulancia(void *arg)
#endif
{
    Veiculo *amb = (Veiculo *)arg;
    Mapa *mapa = (Mapa *)amb->mapa;

    // Tenta ocupar a célula inicial
    Celula *start_cell = via_get_celula(amb->via_atual, amb->index_celula_via);
    if (start_cell != NULL)
    {
        while (!celula_tentar_ocupar(start_cell, amb) && simulation_running && amb->ativo)
        {
            esperar_proximo_tick(global_tick);
        }
        amb->linha = start_cell->linha;
        amb->coluna = start_cell->coluna;
    }

    int last_tick = global_tick;

    while (simulation_running && amb->ativo)
    {
        // Aguarda a sincronização do relógio global (sem busy-wait)
        esperar_proximo_tick(last_tick);
        last_tick = global_tick;

        if (!simulation_running || !amb->ativo)
        {
            break;
        }

        // A ambulância move-se a cada tick (velocidade = 1)
        Via *via = amb->via_atual;
        int idx = amb->index_celula_via;

        // Determina a próxima célula
        int next_idx = idx + 1;
        if (next_idx >= via->num_celulas)
        {
            next_idx = 0; // Wrap around
        }

        Celula *next_cell = via_get_celula(via, next_idx);
        if (next_cell == NULL)
        {
            continue;
        }

        // Verifica se a próxima célula coincide com um cruzamento
        bool next_is_crossing = via_eh_cruzamento(via, next_idx);
        Cruzamento *cruz = NULL;

        if (next_is_crossing)
        {
            cruz = mapa_get_cruzamento(mapa, next_cell->linha, next_cell->coluna);
            if (cruz != NULL)
            {
                // Respeita o semáforo do cruzamento: aguarda o sinal verde normalmente
                // NOTA: A prioridade da ambulância será integrada na próxima etapa (Semana 2)
                pthread_mutex_lock(&cruz->mutex);
                cruzamento_esperar_verde(cruz, via->direcao);
                pthread_mutex_unlock(&cruz->mutex);
            }
        }

        // Tenta ocupar a próxima célula
        if (celula_tentar_ocupar(next_cell, amb))
        {
            // Ocupou com sucesso! Libera a célula anterior
            Celula *curr_cell = via_get_celula(via, idx);
            if (curr_cell != NULL)
            {
                celula_liberar(curr_cell);
            }

            // Atualiza posição da ambulância
            amb->index_celula_via = next_idx;
            amb->linha = next_cell->linha;
            amb->coluna = next_cell->coluna;

            // Se a nova célula ocupada é um cruzamento, decide se vai virar
            if (next_is_crossing && cruz != NULL)
            {
                // Decisão de curva com probabilidade (ex: 30%)
                if ((rand() % 100) < 30)
                {
                    Via *nova_via = (via->direcao == VIA_HORIZONTAL) ? cruz->via_vertical : cruz->via_horizontal;
                    if (nova_via != NULL)
                    {
                        int novo_idx = (nova_via->direcao == VIA_HORIZONTAL) ? next_cell->coluna : next_cell->linha;
                        if (novo_idx >= 0 && novo_idx < nova_via->num_celulas)
                        {
                            amb->via_atual = nova_via;
                            amb->index_celula_via = novo_idx;
                        }
                    }
                }
            }
        }
    }

    // Libera a célula atual ao encerrar
    Celula *curr_cell = via_get_celula(amb->via_atual, amb->index_celula_via);
    if (curr_cell != NULL)
    {
        celula_liberar(curr_cell);
    }

    return 0;
}
