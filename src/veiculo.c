#include "veiculo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* sleep — usado apenas nos stubs temporários */

/* ═══════════════════════════════════════════════════════════
 *  STUBS DE INTEGRAÇÃO
 *  Remova estes blocos conforme Alan e Neto entregarem
 *  as implementações reais.
 * ═══════════════════════════════════════════════════════════ */

int celula_livre(Posicao pos) {
    /* [STUB - Alan] Sempre retorna livre por enquanto */
    (void)pos;
    return 1;
}

void celula_ocupar(Posicao pos, int veiculo_id) {
    /* [STUB - Alan] Sem-op até o mapa existir */
    (void)pos; (void)veiculo_id;
}

void celula_liberar(Posicao pos) {
    /* [STUB - Alan] Sem-op até o mapa existir */
    (void)pos;
}

void aguardar_tick(void) {
    /* [STUB - Neto] Substitua por pthread_cond_wait no relógio global */
    sleep(1);
}

int sinal_verde(Posicao cruzamento, Direcao dir) {
    /* [STUB - Neto] Sempre verde por enquanto */
    (void)cruzamento; (void)dir;
    return 1;
}

void aguardar_sinal_verde(Posicao cruzamento, Direcao dir) {
    /* [STUB - Neto] Substitua por pthread_cond_wait no semáforo de trânsito */
    while (!sinal_verde(cruzamento, dir)) {
        sleep(1); /* temporário — será removido com a integração do Neto */
    }
}

/* ═══════════════════════════════════════════════════════════
 *  FUNÇÕES INTERNAS
 * ═══════════════════════════════════════════════════════════ */

static const char *direcao_str(Direcao d) {
    switch (d) {
        case DIR_NORTE: return "NORTE";
        case DIR_SUL:   return "SUL";
        case DIR_LESTE: return "LESTE";
        case DIR_OESTE: return "OESTE";
        default:        return "?";
    }
}

static const char *tipo_str(TipoVeiculo t) {
    return t == TIPO_AMBULANCIA ? "AMBULANCIA" : "CARRO";
}

/* Calcula a próxima posição com base na direção atual */
static Posicao proxima_posicao(Posicao atual, Direcao dir) {
    Posicao prox = atual;
    switch (dir) {
        case DIR_NORTE: prox.linha--;   break;
        case DIR_SUL:   prox.linha++;   break;
        case DIR_LESTE: prox.coluna++;  break;
        case DIR_OESTE: prox.coluna--;  break;
    }
    return prox;
}

/* Tenta mover o veículo para o próximo passo da rota.
 * Retorna 1 se moveu, 0 se bloqueado (célula ocupada ou sinal). */
static int tentar_mover(Veiculo *v) {
    if (v->rota_idx >= v->rota_tamanho) {
        /* Rota concluída */
        return 0;
    }

    Posicao destino = v->rota[v->rota_idx];

    /* ── 1. Verificar sinal de trânsito ───────────────────── */
    if (!sinal_verde(destino, v->direcao)) {
        v->estado = ESTADO_AGUARDANDO_SINAL;
        printf("[%s #%d] sinal vermelho em (%d,%d) — bloqueando...\n",
               tipo_str(v->tipo), v->id, destino.linha, destino.coluna);

        aguardar_sinal_verde(destino, v->direcao);

        printf("[%s #%d] sinal verde — retomando.\n",
               tipo_str(v->tipo), v->id);
        v->estado = ESTADO_MOVENDO;
    }

    /* ── 2. Verificar se a célula de destino está livre ──── */
    if (!celula_livre(destino)) {
        v->estado = ESTADO_AGUARDANDO_CELULA;
        printf("[%s #%d] célula (%d,%d) ocupada — aguardando...\n",
               tipo_str(v->tipo), v->id, destino.linha, destino.coluna);

        /* Espera bloqueante: tenta novamente no próximo tick.
         * A implementação real usará pthread_cond_wait (Alan/Neto). */
        return 0;
    }

    /* ── 3. Mover: libera célula atual, ocupa a próxima ──── */
    celula_liberar(v->pos);
    celula_ocupar(destino, v->id);

    printf("[%s #%d] (%d,%d) → (%d,%d) [%s]\n",
           tipo_str(v->tipo), v->id,
           v->pos.linha, v->pos.coluna,
           destino.linha, destino.coluna,
           direcao_str(v->direcao));

    v->pos = destino;
    v->rota_idx++;
    v->estado = ESTADO_MOVENDO;
    return 1;
}

/* ─── Função principal da thread ────────────────────────── */
static void *thread_veiculo(void *arg) {
    Veiculo *v = (Veiculo *)arg;

    printf("[%s #%d] iniciado em (%d,%d) vel=%d\n",
           tipo_str(v->tipo), v->id,
           v->pos.linha, v->pos.coluna,
           (int)v->velocidade);

    /* Ocupa a célula inicial */
    celula_ocupar(v->pos, v->id);

    while (v->rota_idx < v->rota_tamanho) {
        /* ── Aguarda o tick do relógio global ─────────────── */
        aguardar_tick();

        /* ── Controle de velocidade ───────────────────────── *
         * Só tenta mover quando tick_contador atingir o valor
         * da velocidade (1, 2 ou 4 ticks).                   */
        v->tick_contador++;
        if (v->tick_contador < (int)v->velocidade) {
            continue;  /* ainda não é hora de mover */
        }
        v->tick_contador = 0;

        /* ── Tenta mover ──────────────────────────────────── */
        tentar_mover(v);
    }

    /* Libera a célula final ao terminar */
    celula_liberar(v->pos);

    v->estado = ESTADO_FINALIZADO;
    printf("[%s #%d] rota concluída. Finalizando thread.\n",
           tipo_str(v->tipo), v->id);

    return NULL;
}

/* ═══════════════════════════════════════════════════════════
 *  API PÚBLICA
 * ═══════════════════════════════════════════════════════════ */

void veiculo_init(Veiculo *v, int id, TipoVeiculo tipo,
                  Posicao pos_inicial, Direcao dir,
                  Velocidade vel, Posicao *rota, int rota_tam) {
    memset(v, 0, sizeof(Veiculo));

    v->id           = id;
    v->tipo         = tipo;
    v->pos          = pos_inicial;
    v->direcao      = dir;
    v->velocidade   = vel;
    v->estado       = ESTADO_MOVENDO;
    v->tick_contador = 0;
    v->rota_idx     = 0;
    v->rota_tamanho = rota_tam > MAX_ROTA ? MAX_ROTA : rota_tam;

    memcpy(v->rota, rota, v->rota_tamanho * sizeof(Posicao));
}

int veiculo_iniciar(Veiculo *v) {
    int ret = pthread_create(&v->thread, NULL, thread_veiculo, v);
    if (ret != 0) {
        fprintf(stderr, "[ERRO] pthread_create falhou para veículo #%d (cod %d)\n",
                v->id, ret);
        return -1;
    }
    return 0;
}

void veiculo_aguardar(Veiculo *v) {
    pthread_join(v->thread, NULL);
}

int veiculos_criar_todos(Veiculo veiculos[], int quantidade) {
    if (quantidade < MIN_VEICULOS || quantidade > MAX_VEICULOS) {
        fprintf(stderr, "[ERRO] quantidade inválida: %d (esperado %d–%d)\n",
                quantidade, MIN_VEICULOS, MAX_VEICULOS);
        return -1;
    }

    int falhas = 0;
    for (int i = 0; i < quantidade; i++) {
        if (veiculo_iniciar(&veiculos[i]) != 0) {
            falhas++;
        }
    }
    return falhas == 0 ? 0 : -1;
}

void veiculos_aguardar_todos(Veiculo veiculos[], int quantidade) {
    for (int i = 0; i < quantidade; i++) {
        veiculo_aguardar(&veiculos[i]);
    }
}