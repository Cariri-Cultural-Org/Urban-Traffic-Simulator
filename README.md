# Simulador de Tráfego Urbano

Simulador em **C** para demonstrar concorrência com threads, mutexes e
variáveis de condição em uma pequena malha de trânsito.

O executável atual cria um mapa real, inicia um relógio global, alterna os
sinais dos cruzamentos, executa veículos em threads e renderiza frames ASCII da
simulação.

## Estado Atual

O `main.c` executa uma demonstração fixa com:

- mapa de `20x40`;
- 7 vias, sendo 3 horizontais e 4 verticais;
- 12 cruzamentos reais;
- 10 veículos em threads, incluindo uma ambulância;
- velocidades de 1, 2 e 4 ticks;
- alternância dos cruzamentos a cada 3 ticks;
- encerramento com `stop`, `broadcast`, `join` e destruição dos recursos.

O binário gerado é:

```bash
bin/traffic-simulator
```

## Como Executar

No Linux, macOS ou WSL:

```bash
make
./bin/traffic-simulator
```

No Windows com MinGW:

```cmd
mingw32-make
.\bin\traffic-simulator.exe
```

Para rodar os testes:

```bash
make test
```

Para remover arquivos gerados:

```bash
make clean
```

São necessários GCC, GNU Make e suporte a `pthreads` em sistemas POSIX ou
MinGW no Windows.

## Estrutura Principal

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
│       ├── Ambulance.c
│       ├── Cell.c
│       ├── CityMap.c
│       ├── CityMapRenderer.c
│       ├── GlobalClock.c
│       ├── Intersection.c
│       ├── Road.c
│       ├── SimulationOutput.c
│       ├── TrafficLight.c
│       ├── Vehicle.c
│       ├── city_map_utils.c
│       └── vehicle_thread.c
├── tests/
│   ├── test_intersection_priority.c
│   └── test_vehicle_invariants.c
├── Makefile
└── README.md
```

## Mapa

O mapa é representado por `CityMap`, definido em `src/models/CityMap.h`.

```c
typedef struct
{
    int rows;
    int columns;
    pthread_mutex_t state_mutex;

    Cell **cells;
    Road **roads;
    int road_count;

    Intersection **intersections;
    int intersection_count;
} CityMap;
```

As dimensões são:

```c
#define CITY_MAP_ROWS 20
#define CITY_MAP_COLUMNS 40
```

O layout atual é fixo:

```text
         col8   col16  col24  col32
           |      |      |      |
row4  ----+------+------+------+----  mão única →
           |      |      |      |
row10 ----+------+------+------+----  mão dupla
           |      |      |      |
row16 ----+------+------+------+----  mão dupla
           |      |      |      |
         dupla  única  dupla  dupla
                  ↓
```

Cada cruzamento é compartilhado pela via horizontal e pela via vertical que se
encontram naquela célula.

## Células

`Cell` representa uma posição do mapa:

```c
typedef struct
{
    int row;
    int column;
    int occupied;
    char occupant_symbol;
    struct Vehicle *vehicle;
    pthread_mutex_t mutex;
} Cell;
```

Cada célula possui um mutex próprio para proteger ocupação, liberação, símbolo
renderizado e ponteiro do ocupante.

O movimento em `vehicle_thread.c` usa também `CityMap.state_mutex` para impedir
que o renderer leia um frame no meio da troca entre célula de origem e célula
de destino. Assim, um frame não deve mostrar o mesmo veículo em duas posições.

## Vias

`Road` guarda ponteiros para células que pertencem ao mapa.

```c
typedef struct
{
    int id;
    RoadDirection direction;
    RoadType type;

    Cell **cells;
    Intersection **intersections;
    int cell_count;
} Road;
```

`ROAD_ONE_WAY` permite avanço apenas no sentido crescente dos índices da via.
`ROAD_TWO_WAY` permite avanço nos dois sentidos na mesma faixa lógica. O
simulador ainda não modela duas faixas físicas independentes para mão dupla.

## Cruzamentos e Sinais

`Intersection` é a fonte de verdade para os sinais usados pela simulação.

```c
typedef struct Intersection
{
    int id;
    int row;
    int column;
    Road *horizontal_road;
    Road *vertical_road;

    RoadDirection green_direction;
    RoadDirection previous_green_direction;
    pthread_mutex_t mutex;
    pthread_cond_t horizontal_cond;
    pthread_cond_t vertical_cond;

    int ambulance_present;
    RoadDirection ambulance_direction;
} Intersection;
```

Quando uma ambulância solicita prioridade, o cruzamento guarda a direção verde
anterior, libera a direção da ambulância e restaura o ciclo normal quando a
prioridade é limpa.

O módulo `TrafficLight` permanece no projeto como componente didático, mas a
simulação integrada usa `Intersection.green_direction`.

## Relógio Global

`GlobalClock.c` mantém:

```c
int global_tick;
os_mutex_t clock_mutex;
os_cond_t clock_cond;
bool simulation_running;
```

A thread do relógio incrementa o tick a cada `1000 ms` e acorda as threads que
estão esperando o próximo ciclo. O encerramento chama `stop_global_clock()`,
faz broadcast nas condições e aguarda as threads com `join`.

## Veículos

Os veículos da demonstração usam `ThreadVehicle`, definido em
`src/models/vehicle_thread.h`.

O avanço integrado valida:

- destino dentro do mapa;
- movimento para célula adjacente;
- direção do veículo compatível com a via;
- regra de mão única;
- sinal verde antes de entrar em cruzamento;
- prioridade de ambulância;
- célula de destino livre.

O veículo não segura mutex de célula enquanto espera tick ou sinal. A troca de
ocupação entre origem e destino acontece sob o mutex do mapa e os mutexes das
células envolvidas.

## Testes

`make test` compila e executa:

- `test_intersection_priority`: prioridade da ambulância, restauração do ciclo
  do cruzamento e destruição de recursos;
- `test_vehicle_invariants`: avanço sem duplicação final, bloqueio atrás de
  veículo ocupado, mão única, mão dupla e limite do mapa.

## Limitações

- As rotas da demonstração são fixas no código.
- Não há entrada por linha de comando para configurar número de veículos,
  duração ou mapa.
- Mão dupla ainda é uma única faixa lógica onde o sentido reverso é aceito; não
  há duas faixas físicas independentes.
- O renderer ASCII é uma visualização simples para debug e demonstração.
- O projeto não roda Valgrind ou sanitizers automaticamente pelo `Makefile`.

## Documentação Complementar

- [Explicação do projeto](docs/explicacao_projeto.md)
- [Divisão de tarefas](docs/tasks-division.md)
- [Conceitos de Sistemas Operacionais](docs/conceitos_so.md)
- [Memória virtual](docs/memoria_virtual.md)
