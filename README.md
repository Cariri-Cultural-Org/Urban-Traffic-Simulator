# Simulador de Tráfego Urbano

Este projeto simula o funcionamento de uma pequena malha de trânsito utilizando
**C**, **threads**, **mutexes** e **variáveis de condição**.

A ideia principal é representar a cidade como uma matriz de células. As ruas
são formadas por conjuntos dessas células, e os veículos devem percorrê-las sem
ocupar a mesma posição ao mesmo tempo.

O projeto foi desenvolvido para a disciplina de Sistemas Operacionais e tem
como foco o estudo de concorrência, sincronização de recursos e prevenção de
condições de corrida.

## Como o projeto funciona

A lógica principal pode ser dividida em cinco partes:

1. Estrutura do mapa
2. Células e vias
3. Cruzamentos e semáforos
4. Relógio global
5. Veículos e sincronização

## Estrutura do mapa

O mapa é representado pela estrutura `Mapa`, definida em
`src/models/Mapa.h`.

```c
typedef struct
{
    int linhas;
    int colunas;

    Celula **celulas;
    Via **vias;
    int num_vias;

    Cruzamento **cruzamentos;
    int num_cruzamentos;
} Mapa;
```

As dimensões são definidas pelas constantes:

```c
#define MAPA_LINHAS 20
#define MAPA_COLUNAS 40
```

Isso significa que a cidade possui uma matriz de `20x40`, totalizando `800`
posições.

Cada posição representa uma `Celula`. As vias e os cruzamentos utilizam essas
células para organizar a malha viária.

## Layout da malha viária

O layout do mapa é fixo. Existem três vias horizontais e quatro vias verticais.

```text
         col8   col16  col24  col32
           |      |      |      |
lin4  ----+------+------+------+----  mão única →
           |      |      |      |
lin10 ----+------+------+------+----  mão dupla
           |      |      |      |
lin16 ----+------+------+------+----  mão dupla
           |      |      |      |
         dupla  única  dupla  dupla
                  ↓
```

As vias horizontais estão localizadas nas linhas `4`, `10` e `16`.

```c
static const int LINHAS_H[] = {4, 10, 16};
```

As vias verticais estão localizadas nas colunas `8`, `16`, `24` e `32`.

```c
static const int COLUNAS_V[] = {8, 16, 24, 32};
```

A linha 4 é uma via de mão única da esquerda para a direita. A coluna 16 também
é de mão única, de cima para baixo. As demais vias são configuradas como mão
dupla.

Cada uma das três vias horizontais cruza as quatro vias verticais. Portanto, o
mapa possui:

```text
3 vias horizontais × 4 vias verticais = 12 cruzamentos
```

## Células

A estrutura `Celula` representa uma posição do mapa.

```c
typedef struct
{
    int linha;
    int coluna;
    int ocupada;
    struct Veiculo *veiculo;
    pthread_mutex_t mutex;
} Celula;
```

Cada célula armazena:

- sua linha e sua coluna;
- a informação de ocupação;
- um ponteiro para o veículo presente;
- um mutex para proteger o acesso concorrente.

Quando um veículo tenta avançar, a célula de destino precisa estar livre. A
operação de ocupação utiliza o mutex da célula para impedir que duas threads
ocupem a mesma posição simultaneamente.

De forma simplificada, a lógica é:

```text
bloquear mutex da célula
    ↓
verificar se está livre
    ↓
registrar o veículo
    ↓
liberar mutex
```

A função `celula_tentar_ocupar()` realiza a tentativa de ocupação, enquanto
`celula_liberar()` libera a posição anterior do veículo.

## Vias

Uma `Via` representa uma rua horizontal ou vertical.

```c
typedef struct
{
    int id;
    DirecaoVia direcao;
    SentidoVia sentido;

    Celula **celulas;
    Cruzamento **cruzamentos;
    int num_celulas;
} Via;
```

As células de uma via não são cópias separadas. A estrutura armazena ponteiros
para células que já pertencem ao mapa.

Por exemplo, a via horizontal da linha 4 utiliza as posições:

```text
(4,0) → (4,1) → (4,2) → ... → (4,39)
```

A via vertical da coluna 8 utiliza:

```text
(0,8)
  ↓
(1,8)
  ↓
 ...
  ↓
(19,8)
```

Quando essas vias se encontram em `(4,8)`, ambas apontam para a mesma célula.
Essa posição também recebe um objeto `Cruzamento`.

No estado atual, o tipo da via registra se ela é de mão única ou mão dupla. A
representação completa de duas faixas independentes em sentidos opostos ainda
está em desenvolvimento.

## Cruzamentos

Um `Cruzamento` conecta uma via horizontal e uma via vertical.

```c
typedef struct Cruzamento
{
    int id;
    int linha;
    int coluna;
    Via *via_h;
    Via *via_v;

    DirecaoVia direcao_verde;
    pthread_mutex_t mutex;
    pthread_cond_t cond_h;
    pthread_cond_t cond_v;

    int ambulancia_presente;
    DirecaoVia direcao_ambulancia;
} Cruzamento;
```

O campo `direcao_verde` informa qual fluxo pode atravessar o cruzamento. Se a
direção horizontal estiver verde, a vertical deve aguardar, e vice-versa.

As variáveis de condição `cond_h` e `cond_v` permitem bloquear as threads dos
veículos sem consumir processamento enquanto o sinal correspondente estiver
vermelho.

De forma simplificada:

```text
veículo chega ao cruzamento
    ↓
verifica a direção liberada
    ↓
sinal vermelho → aguarda a variável de condição
sinal verde    → tenta ocupar a próxima célula
```

A estrutura também possui campos reservados para conceder prioridade à
ambulância.

## Relógio global

O relógio global coordena a passagem do tempo da simulação.

```c
int global_tick = 0;
os_mutex_t clock_mutex;
os_cond_t clock_cond;
bool simulation_running = true;
```

A thread do relógio aguarda aproximadamente `100ms`, incrementa o tick e acorda
as threads que estão esperando o próximo ciclo.

```c
usleep(100000);
pthread_mutex_lock(&clock_mutex);

global_tick++;
pthread_cond_broadcast(&clock_cond);

pthread_mutex_unlock(&clock_mutex);
```

Assim, um tick funciona como uma unidade de tempo compartilhada. Veículos,
semáforos e outros componentes podem aguardar o próximo tick por meio da função
`esperar_proximo_tick()`.

O uso de variável de condição evita um loop verificando continuamente o valor
do relógio.

## Semáforos

O módulo `Semaforo` armazena o estado atual e a duração dos períodos verde e
vermelho.

```c
if (sem->estado == VERMELHO &&
    sem->tick_atual >= sem->tempo_vermelho)
{
    sem->estado = VERDE;
    sem->tick_atual = 0;
}
```

A cada tick, `atualizar_semaforo()` incrementa o contador interno. Quando o
tempo configurado é alcançado, o estado é alternado e o contador volta para
zero.

Na execução atual de `main.c`, um semáforo de demonstração é atualizado durante
10 ticks. A integração completa entre relógio, mapa, cruzamentos e veículos
ainda está em desenvolvimento.

## Veículos e sincronização

Os veículos são os agentes que percorrem as vias. Cada veículo deve executar em
sua própria thread e compartilhar as células do mapa com os demais.

Para um movimento ser seguro, a simulação precisa garantir que:

- a célula de destino pertence à rota;
- o veículo respeita o sentido da via;
- o sinal do cruzamento está verde;
- a célula de destino está livre;
- dois veículos não ocupam a mesma célula;
- nenhum mutex permanece bloqueado durante a espera por tick ou sinal.

O fluxo esperado de movimento é:

```text
aguardar o próximo tick
    ↓
verificar sinal e rota
    ↓
tentar ocupar a célula seguinte
    ↓
liberar a célula anterior
    ↓
atualizar a posição do veículo
```

Quando mais de um mutex precisar ser adquirido, todos os módulos devem seguir
uma ordem determinística. Essa regra reduz o risco de espera circular e
deadlock.

## Estrutura dos arquivos

```text
.
├── docs/
│   ├── conceitos_so.md
│   ├── explicacao_projeto.md
│   ├── memoria_virtual.md
│   └── tasks-division.md
├── src/
│   ├── main.c
│   └── models/
│       ├── Celula.c
│       ├── Cruzamento.c
│       ├── Mapa.c
│       ├── Relogio_global.c
│       ├── Semaforo.c
│       ├── Via.c
│       └── Veiculo.c
├── Makefile
└── README.md
```

## Como executar o projeto

O projeto utiliza vários arquivos-fonte. Por isso, ele deve ser compilado pelo
`Makefile` a partir da pasta raiz.

No Linux, macOS ou WSL:

```bash
make
./bin/simulador
```

No Windows com MinGW:

```cmd
mingw32-make
.\bin\simulador.exe
```

Para remover os arquivos gerados:

```bash
make clean
```

São necessários GCC, GNU Make e suporte a `pthreads` em sistemas POSIX ou MinGW
no Windows.

## Orientações para participantes

Antes de modificar uma estrutura compartilhada, verifique os contratos
declarados nos arquivos `.h` e os módulos que utilizam essa estrutura.

Ao implementar uma funcionalidade concorrente:

- proteja toda leitura e escrita de estado compartilhado;
- utilize variáveis de condição para espera bloqueante;
- não aguarde um tick ou sinal segurando o mutex de uma célula;
- mantenha uma ordem única para aquisição de múltiplos mutexes;
- finalize e aguarde as threads antes de destruir seus recursos;
- compile o projeto e valide o comportamento após cada alteração.

Mais detalhes estão disponíveis na
[explicação do projeto](docs/explicacao_projeto.md) e na
[divisão de tarefas](docs/tasks-division.md).

## Resumo da lógica

O projeto funciona da seguinte forma:

- uma matriz de `20x40` representa a cidade;
- cada posição da matriz é uma célula protegida por mutex;
- as vias armazenam ponteiros para as células do mapa;
- três vias horizontais e quatro verticais formam 12 cruzamentos;
- os cruzamentos controlam qual direção pode avançar;
- o relógio global produz um tick aproximadamente a cada `100ms`;
- variáveis de condição acordam as threads sem espera ocupada;
- os veículos tentam ocupar a próxima célula antes de liberar a anterior;
- a sincronização impede ocupações simultâneas e reduz o risco de deadlocks;
- a ambulância utiliza a mesma malha, com suporte para prioridade nos
  cruzamentos.

## Documentação complementar

- [Explicação do projeto](docs/explicacao_projeto.md)
- [Divisão de tarefas](docs/tasks-division.md)
- [Conceitos de Sistemas Operacionais](docs/conceitos_so.md)
- [Memória virtual](docs/memoria_virtual.md)
