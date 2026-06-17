#ifndef VEICULO_H
#define VEICULO_H

#include <pthread.h>

/* ─── Constantes ─────────────────────────────────────────── */
#define MAX_VEICULOS     20
#define MIN_VEICULOS     10
#define MAX_ROTA         256

/* ─── Enums ──────────────────────────────────────────────── */
typedef enum {
    DIR_NORTE,
    DIR_SUL,
    DIR_LESTE,
    DIR_OESTE
} Direcao;

typedef enum {
    VEL_RAPIDO = 1,   /* move a cada 1 tick  */
    VEL_MEDIO  = 2,   /* move a cada 2 ticks */
    VEL_LENTO  = 4    /* move a cada 4 ticks */
} Velocidade;

typedef enum {
    TIPO_CARRO,
    TIPO_AMBULANCIA
} TipoVeiculo;

typedef enum {
    ESTADO_MOVENDO,
    ESTADO_AGUARDANDO_CELULA,   /* bloqueado esperando célula livre   */
    ESTADO_AGUARDANDO_SINAL,    /* bloqueado no sinal vermelho        */
    ESTADO_FINALIZADO
} EstadoVeiculo;

/* ─── Posição ────────────────────────────────────────────── */
typedef struct {
    int linha;
    int coluna;
} Posicao;

/* ─── Estrutura principal ────────────────────────────────── */
typedef struct {
    int          id;
    TipoVeiculo  tipo;
    Posicao      pos;
    Direcao      direcao;
    Velocidade   velocidade;
    EstadoVeiculo estado;

    /* rota: sequência de posições que o veículo deve percorrer */
    Posicao      rota[MAX_ROTA];
    int          rota_tamanho;
    int          rota_idx;       /* próximo passo da rota */

    /* controle de tick: veículo só move quando tick_contador == velocidade */
    int          tick_contador;

    pthread_t    thread;
} Veiculo;

/* ─── API pública ────────────────────────────────────────── */

/* Inicializa um veículo com os parâmetros fornecidos */
void veiculo_init(Veiculo *v, int id, TipoVeiculo tipo,
                  Posicao pos_inicial, Direcao dir,
                  Velocidade vel, Posicao *rota, int rota_tam);

/* Cria a pthread do veículo e começa a simulação */
int veiculo_iniciar(Veiculo *v);

/* Aguarda a thread do veículo terminar */
void veiculo_aguardar(Veiculo *v);

/* Cria e inicia entre MIN_VEICULOS e MAX_VEICULOS veículos */
int veiculos_criar_todos(Veiculo veiculos[], int quantidade);

/* Aguarda todos os veículos finalizarem */
void veiculos_aguardar_todos(Veiculo veiculos[], int quantidade);

/* ─── Stubs de integração (serão substituídos depois) ───── */

/* [STUB - Alan] Retorna 1 se a célula está livre, 0 caso contrário */
int celula_livre(Posicao pos);

/* [STUB - Alan] Ocupa a célula para o veículo de id fornecido */
void celula_ocupar(Posicao pos, int veiculo_id);

/* [STUB - Alan] Libera a célula */
void celula_liberar(Posicao pos);

/* [STUB - Neto] Bloqueia a thread até o próximo tick do relógio global */
void aguardar_tick(void);

/* [STUB - Neto] Retorna 1 se o sinal está verde para a direção dada */
int sinal_verde(Posicao cruzamento, Direcao dir);

/* [STUB - Neto] Bloqueia a thread até o sinal ficar verde */
void aguardar_sinal_verde(Posicao cruzamento, Direcao dir);

#endif /* VEICULO_H */