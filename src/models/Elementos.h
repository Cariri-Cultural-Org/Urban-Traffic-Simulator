/*
 * elements_ascii.h
 * ─────────────────────────────────────────────────────────────────
 * Simulador de Tráfego Urbano — Sistemas Operacionais
 * Protótipo visual em ASCII: todos os elementos gráficos do projeto.
 *
 * Como usar:
 *   #include "elements_ascii.h"
 *
 * Convenções:
 *   - Semáforo VERMELHO  → [R]   (Red   — via fechada)
 *   - Semáforo VERDE     → [G]   (Green — via aberta)
 *   - Carro comum        → /---\ com C no interior
 *   - Ambulância         → /###\ com A no interior
 *   - Setas de fluxo     → << (Oeste/Sul)  >> (Leste/Norte)
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef ELEMENTS_ASCII_H
#define ELEMENTS_ASCII_H


/*
 * 1. Células Básicas Do Mapa
 */

#define CELULA_RUA_H        "═══"   /* Rua horizontal               */
#define CELULA_RUA_V        " │ "   /* Rua vertical                 */
#define CELULA_CRUZAMENTO   "─┼─"   /* Interseção de vias           */
#define CELULA_VAZIA        "   "   /* Espaço sem via               */
#define CELULA_CALCADA      "▒▒▒"   /* Calçada / faixa de pedestre  */
#define CELULA_EDIFICIO     "███"   /* Bloco de edifício            */
#define CELULA_PARQUE       " T "   /* Área verde (T = árvore)      */
#define CELULA_CASA         "_H_"   /* Zona residencial             */


/*
 * 2. Faixas De Tráfego
 */

/* Mão única — fluxo para Oeste (esquerda) */
#define FAIXA_OESTE     " << Faixa Unica (Oeste) << "

/* Mão dupla — faixa sentido Leste */
#define FAIXA_LESTE_N   " >> Faixa Norte >> "   /* faixa de cima */
#define FAIXA_LESTE_S   " >> Faixa Sul   >> "   /* faixa de baixo */

/* Mão dupla — faixa sentido Norte (subindo) */
#define FAIXA_NORTE     " ^^ Faixa Norte ^^ "

/* Mão dupla — faixa sentido Sul (descendo) */
#define FAIXA_SUL       " vv Faixa Sul   vv "


/*
 * 3. Semáforos
 */

/*
 * Estado A: vertical ABERTA, horizontal FECHADA
 *
 *        │   │   │
 *        │ ^ │ v │
 *  ──────┘   └───┘──────
 *   << [R]         [R] <<
 *  ──────┐         ┌──────
 *   >> [R]         [R] >>
 *  ──────┘   ┌───┐──────
 *        │[G]│ v │
 *        │   │   │
 */
#define SEMAFORO_V_ABERTO_H_FECHADO \
    "       |   |   |\n" \
    "       | ^ | v |\n" \
    " ------+   +---+------\n" \
    "  << [R]         [R] <<\n" \
    " ------+         +------\n" \
    "  >> [R]         [R] >>\n" \
    " ------+   +---+------\n" \
    "       |[G]| v |\n" \
    "       |   |   |"

/*
 * Estado B: horizontal ABERTA, vertical FECHADA
 *
 *        │   │   │
 *        │ ^ │ v │
 *  ──────┘   └───┘──────
 *   << [G]         [G] <<
 *  ──────┐         ┌──────
 *   >> [G]         [G] >>
 *  ──────┘   ┌───┐──────
 *        │[R]│ v │
 *        │   │   │
 */
#define SEMAFORO_H_ABERTO_V_FECHADO \
    "       |   |   |\n" \
    "       | ^ | v |\n" \
    " ------+   +---+------\n" \
    "  << [G]         [G] <<\n" \
    " ------+         +------\n" \
    "  >> [G]         [G] >>\n" \
    " ------+   +---+------\n" \
    "       |[R]| v |\n" \
    "       |   |   |"


/*
 * 4. Veículos - Carros Comuns
 */

/*  Indo para Norte (subindo)
 *    /---\
 *    | C |
 *    \___/
 */
#define CARRO_NORTE \
    " /---\\\n" \
    " | C |\n" \
    " \\___/"

/*  Indo para Sul (descendo)
 *    /___\
 *    | C |
 *    \---/
 */
#define CARRO_SUL \
    " /___\\\n" \
    " | C |\n" \
    " \\---/"

/*  Indo para Leste (direita)
 *    +------+
 *    |C C C >
 *    +------+
 */
#define CARRO_LESTE \
    " +------+\n" \
    " |C C C >\n" \
    " +------+"

/*  Indo para Oeste (esquerda)
 *    +------+
 *    < C C C|
 *    +------+
 */
#define CARRO_OESTE \
    " +------+\n" \
    " < C C C|\n" \
    " +------+"


/*
 * 5. Veículos - Ambulância
 */

/*  Indo para Norte (subindo)
 *    /###\
 *    |[+]|
 *    | A |
 *    \___/
 */
#define AMBULANCIA_NORTE \
    " /###\\\n" \
    " |[+]|\n" \
    " | A |\n" \
    " \\___/"

/*  Indo para Sul (descendo)
 *    /___\
 *    | A |
 *    |[+]|
 *    \###/
 */
#define AMBULANCIA_SUL \
    " /___\\\n" \
    " | A |\n" \
    " |[+]|\n" \
    " \\###/"

/*  Indo para Leste (direita)
 *    +--------+
 *    |A  [+]  >
 *    +--------+
 */
#define AMBULANCIA_LESTE \
    " +--------+\n" \
    " |A  [+]  >\n" \
    " +--------+"

/*  Indo para Oeste (esquerda)
 *    +--------+
 *    <  [+]  A|
 *    +--------+
 */
#define AMBULANCIA_OESTE \
    " +--------+\n" \
    " <  [+]  A|\n" \
    " +--------+"


/*
 * 6. Cenário Urbano - Blocos Decorativos
 */

/*
 * Edifício Empresarial (bloco à esquerda do mapa)
 *
 *  +-------------------------------+
 *  | [Edificio Empresarial]        |
 *  | ### ### ### ### ###           |
 *  | # # # # # # # # # #          |
 *  | ### ### ### ### ###           |
 *  +-------------------------------+
 */
#define EDIFICIO \
    " +-------------------------------+\n" \
    " | [Edificio Empresarial]        |\n" \
    " | ### ### ### ### ###           |\n" \
    " | # # # # # # # # # #          |\n" \
    " | ### ### ### ### ###           |\n" \
    " +-------------------------------+"

/*
 * Zona Residencial (bloco à direita do mapa)
 *
 *  +---------------+ +---------------+
 *  |   _/\_        | |   _/\_        |
 *  |  /____\       | |  /____\       |
 *  |  | H  |       | |  | H  |       |
 *  +---------------+ +---------------+
 */
#define ZONA_RESIDENCIAL \
    " +---------------+ +---------------+\n" \
    " |   _/\\_        | |   _/\\_        |\n" \
    " |  /____\\       | |  /____\\       |\n" \
    " |  | H  |       | |  | H  |       |\n" \
    " +---------------+ +---------------+"

/*
 * Parque Urbano (bloco inferior direito)
 *
 *  +---------------------------------+
 *  | [PARQUE URBANO]  ### Pista      |
 *  |  T   T   T       ### Caminhada  |
 *  |    T   T   T                    |
 *  +---------------------------------+
 */
#define PARQUE_URBANO \
    " +---------------------------------+\n" \
    " | [PARQUE URBANO]  ### Pista      |\n" \
    " |  T   T   T       ### Caminhada  |\n" \
    " |    T   T   T                    |\n" \
    " +---------------------------------+"

/*
 * Área Verde / Bosque (bloco inferior esquerdo)
 *
 *  +-------------------------------+
 *  |  T   T   T   T   T   T       |
 *  |    T   T   T   T   T   T     |
 *  +-------------------------------+
 */
#define BOSQUE \
    " +-------------------------------+\n" \
    " |  T   T   T   T   T   T       |\n" \
    " |    T   T   T   T   T   T     |\n" \
    " +-------------------------------+"


/*
 * 7. Frames De Animação - Exemplo De Ticks
 */

/*
 * TICK 01 — Carro comum desce pelo eixo vertical.
 *            Ambulância aguarda abaixo, no eixo vertical.
 *            Semáforo horizontal: [R] | Semáforo vertical: [G]
 *
 *      |   |     |   |
 *      |   |/---\|   |
 *      |   || C ||   |
 *   ---+   |\___/+   +---
 *   << [R]       [R] <<
 *   ---+         +---
 *   >> [R]       [R] >>
 *   ---+   |     |   +---
 *      |   |     |   |
 *      |/###\    |   |
 *      || A |    |   |
 */
#define TICK_01 \
    "      |   |     |   |\n" \
    "      |   |/---\\|   |\n" \
    "      |   || C ||   |\n" \
    "   ---+   |\\___/|   +---\n" \
    "   << [R]         [R] <<\n" \
    "   ---+           +---\n" \
    "   >> [R]         [R] >>\n" \
    "   ---+   |     |   +---\n" \
    "      |   |     |   |\n" \
    "      |/###\\    |   |\n" \
    "      || A |    |   |"

/*
 * TICK 02 — Carro comum entra no cruzamento.
 *            Ambulância avança para o cruzamento.
 *            Semáforo horizontal: [R] | Semáforo vertical: [G]
 *
 *      |   |     |   |
 *      |   |     |   |
 *      |   |     |   |
 *   ---+   |/---\|   +---
 *   << [R] || C ||[R] <<
 *   ---+   |\___/|   +---
 *   >> [R]       [R] >>
 *   ---+   |/###\|   +---
 *      |   || A ||   |
 *      |   || + ||   |
 */
#define TICK_02 \
    "      |   |     |   |\n" \
    "      |   |     |   |\n" \
    "      |   |     |   |\n" \
    "   ---+   |/---\\|   +---\n" \
    "   << [R] || C ||[R] <<\n" \
    "   ---+   |\\___/|   +---\n" \
    "   >> [R]         [R] >>\n" \
    "   ---+   |/###\\|   +---\n" \
    "      |   || A ||   |\n" \
    "      |   || + ||   |"

/*
 * TICK 03 — Carro comum sai do cruzamento (sul).
 *            Ambulância ocupa o cruzamento com prioridade.
 *            Carro horizontal entra pela faixa leste.
 *            Semáforo: ambulância força verde vertical.
 *
 *      |   |     |   |
 *      |   |     |   |
 *      |   |/---\|   |
 *   ---+   || C ||   +---
 *  +------+|\___/|
 *  |C C C >| [R] |  [G] <<
 *  +------+|/###\|
 *   >> [G] || A || [R] >>
 *   ---+   || + ||   +---
 *      |   |\___/|   |
 */
#define TICK_03 \
    "      |   |     |   |\n" \
    "      |   |     |   |\n" \
    "      |   |/---\\|   |\n" \
    "   ---+   || C ||   +---\n" \
    "  +------+|\\___/|\n" \
    "  |C C C >|  [R]|      [G] <<\n" \
    "  +------+|/###\\|\n" \
    "   >> [G] || A ||  [R] >>\n" \
    "   ---+   || + ||   +---\n" \
    "      |   |\\___/|   |"


/*
 * 8. Legenda
 */

#define LEGENDA \
    "+----------------------------------------------------------+\n" \
    "| Simbolo  | Significado        | Funcao Logica            |\n" \
    "+----------+--------------------+--------------------------+\n" \
    "| ### / _H_| Fachadas / Casas   | Barreira fisica.         |\n" \
    "| ###      | Faixa de Seguranca | Travessia de pedestres.  |\n" \
    "| ^ v < >  | Setas de Fluxo     | Sentido obrigatorio.     |\n" \
    "| [R]      | Semaforo Vermelho  | Veiculos param e dormem. |\n" \
    "| [G]      | Semaforo Verde     | Fluxo livre.             |\n" \
    "| /---\\    | Carro Comum  (C)   | Respeita semaforos.      |\n" \
    "| /###\\    | Ambulancia   (A)   | Prioridade nos sinais.   |\n" \
    "+----------------------------------------------------------+"


/*
 * 9. Identificadores De Veículos Na Grade
 */

#define ID_CARRO        'C'
#define ID_AMBULANCIA   'A'
#define ID_VAZIO        ' '
#define ID_PAREDE       '#'
#define ID_RUA          '.'
#define ID_CRUZAMENTO   '+'
#define ID_SEM_VERDE    'G'
#define ID_SEM_VERM     'R'


#endif /* ELEMENTS_ASCII_H */