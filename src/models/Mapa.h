
#ifndef MAPA_H
#define MAPA_H

#include "Celula.h"
#include "Via.h"
#include "Cruzamento.h"

/* Dimensões padrão da malha viária (podem ser ajustadas). */
#define MAPA_LINHAS 20
#define MAPA_COLUNAS 40

/* Quantidade mínima de cruzamentos exigida pelo enunciado. */
#define MAPA_MIN_CRUZAMENTOS 8

/*
 * Mapa é o container central da simulação.
 * Agrega a matriz de células, todas as vias e todos os cruzamentos.
 * É o único struct compartilhado por TODAS as threads — por isso cada
 * célula e cada cruzamento possuem seus próprios mutexes.
 *
 * O Mapa não precisa de um mutex próprio porque:
 *   - a estrutura (dimensões, vetores) é somente-leitura após a init
 *   - o acesso concorrente às células é protegido pelo mutex de cada Celula
 *   - o acesso aos cruzamentos é protegido pelo mutex de cada Cruzamento
 */
typedef struct
{
    int linhas;  /* número de linhas  da matriz                 */
    int colunas; /* número de colunas da matriz                 */

    /*
     * Matriz de células: mapa->celulas[l][c] é a célula na linha l, coluna c.
     * Alocada dinamicamente em mapa_init().
     */
    Celula **celulas;

    /* Vetor de todas as vias do mapa. */
    Via **vias;
    int num_vias;

    /* Vetor de todos os cruzamentos do mapa. */
    Cruzamento **cruzamentos;
    int num_cruzamentos;

} Mapa;

/* ---------- Funções auxiliares (implementadas em Mapa.c) ---------- */

/*
 * Aloca e inicializa o mapa completo:
 *   - aloca a matriz de Celulas e inicializa cada uma
 *   - cria as vias horizontais e verticais
 *   - cria os cruzamentos e os conecta às vias correspondentes
 *   - inicializa os mutexes de todas as células e cruzamentos
 *
 * Retorna o ponteiro para o Mapa alocado, ou NULL em caso de falha.
 */
Mapa *mapa_criar(void);

/*
 * Libera toda a memória do mapa:
 *   - destrói mutexes de células e cruzamentos
 *   - libera vias e seus vetores internos
 *   - libera a matriz de células
 *   - libera o próprio Mapa
 */
void mapa_destruir(Mapa *m);

/*
 * Retorna o ponteiro para a célula em (linha, coluna).
 * Retorna NULL se as coordenadas estiverem fora dos limites.
 */
Celula *mapa_get_celula(const Mapa *m, int linha, int coluna);

/*
 * Retorna o ponteiro para o cruzamento localizado em (linha, coluna),
 * ou NULL se não houver cruzamento nessa posição.
 */
Cruzamento *mapa_get_cruzamento(const Mapa *m, int linha, int coluna);

/*
 * Verifica se a posição (linha, coluna) está dentro dos limites do mapa.
 * Retorna 1 se válida, 0 caso contrário.
 */
int mapa_posicao_valida(const Mapa *m, int linha, int coluna);

#endif /* MAPA_H */
