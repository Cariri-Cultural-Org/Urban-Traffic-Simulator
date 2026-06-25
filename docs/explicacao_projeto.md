# Explicação do Projeto: Simulador de Tráfego Urbano

> **Disciplina:** Sistemas Operacionais
>
> **Linguagem:** C
>
> **Assunto principal:** concorrência com threads, mutexes e variáveis de condição
>
> **Estado do código descrito neste documento:** 24/06/2026

---

## 1. O projeto em uma frase

Este projeto tenta representar uma pequena cidade como uma grade de células na qual veículos se movimentam ao mesmo tempo, sem ocupar o mesmo espaço e respeitando o relógio e os semáforos.

A palavra mais importante é **“ao mesmo tempo”**. A finalidade acadêmica do projeto não é apenas desenhar carros no terminal, mas praticar como vários fluxos de execução compartilham dados sem causar conflitos.

---

## 2. Primeiro, imagine uma cidade feita de quadrados

O mapa é uma matriz com:

- 20 linhas;
- 40 colunas;
- uma `Celula` em cada posição.

Uma célula é o menor espaço do mapa. Ela pode estar livre ou ocupada por um veículo.

```text
Mapa
└── 20 linhas × 40 colunas
    └── cada posição é uma Celula
        ├── conhece sua linha e sua coluna
        ├── informa se está ocupada
        ├── aponta para o veículo que a ocupa
        └── possui um mutex
```

As ruas, chamadas de `Via`, não possuem células separadas. Cada via guarda ponteiros para células que já pertencem ao mapa.

Por exemplo, a via horizontal da linha 4 aponta para:

```text
(4,0) → (4,1) → (4,2) → ... → (4,39)
```

A via vertical da coluna 8 aponta para:

```text
(0,8)
  ↓
(1,8)
  ↓
 ...
  ↓
(19,8)
```

Quando as duas vias se encontram em `(4,8)`, ambas apontam para a **mesma célula do mapa**. Essa posição também recebe um objeto `Cruzamento`, que controla os sinais das duas direções.

Essa relação pode ser resumida assim:

```text
Mapa
├── possui todas as Celulas
├── possui as Vias
│   └── cada Via aponta para uma sequência de Celulas do Mapa
└── possui os Cruzamentos
    └── cada Cruzamento conecta uma Via horizontal e uma Via vertical
```

---

## 3. Como é a malha viária

O mapa construído em `Mapa.c` possui três vias horizontais e quatro verticais:

```text
               col. 8   col. 16   col. 24   col. 32
                  │         │          │          │
linha 4   ─────────┼─────────┼──────────┼──────────┼────
                  │         │          │          │
linha 10  ─────────┼─────────┼──────────┼──────────┼────
                  │         │          │          │
linha 16  ─────────┼─────────┼──────────┼──────────┼────
                  │         │          │          │
```

Como cada uma das três vias horizontais cruza as quatro verticais, existem:

```text
3 × 4 = 12 cruzamentos
```

No código:

- a linha 4 é marcada como mão única;
- as linhas 10 e 16 são marcadas como mão dupla;
- a coluna 16 é marcada como mão única;
- as colunas 8, 24 e 32 são marcadas como mão dupla.

### Atenção sobre as vias de mão dupla

O tipo `Via` possui o campo `sentido`, mas a implementação atual cria apenas uma sequência de células para cada rua. Ela ainda não cria duas faixas independentes nem implementa veículos andando em sentidos opostos.

Portanto, no estado atual, “mão dupla” é principalmente uma informação armazenada na estrutura, não um comportamento completo da simulação.

---

## 4. Qual é o papel de cada arquivo

### `src/main.c`

É o ponto de entrada do programa. Atualmente ele:

1. inicializa o relógio global;
2. cria um único `Semaforo` de teste;
3. inicia a thread do relógio;
4. espera e imprime 10 ticks;
5. atualiza o semáforo a cada tick;
6. encerra a thread e libera os recursos.

O arquivo ainda **não cria o mapa, os cruzamentos, os veículos ou a ambulância**.

### `src/models/Celula.h` e `Celula.c`

Representam uma posição da grade.

Campos principais:

| Campo | Significado |
|---|---|
| `linha`, `coluna` | Coordenadas da célula |
| `ocupada` | `0` se livre e `1` se ocupada |
| `veiculo` | Ponteiro para quem está ocupando a célula |
| `mutex` | Protege a ocupação e a liberação |

Operações importantes:

- `celula_tentar_ocupar`: ocupa somente se a célula estiver livre;
- `celula_liberar`: marca a célula como livre;
- `celula_init` e `celula_destroy`: inicializam e destroem o mutex.

### `src/models/Via.h` e `Via.c`

Representam uma rua como uma sequência ordenada de ponteiros para células.

Uma via sabe:

- seu identificador;
- se é horizontal ou vertical;
- se foi classificada como mão única ou mão dupla;
- quais células formam seu percurso;
- em quais índices existem cruzamentos.

O índice da célula é importante porque o movimento atual é calculado com:

```c
next_idx = idx + 1;
```

Ou seja, o veículo avança para a próxima posição do vetor da via.

### `src/models/Mapa.h` e `Mapa.c`

O `Mapa` é o contêiner central da cidade. Ele cria e agrupa:

- a matriz de 800 células;
- 7 vias;
- 12 cruzamentos.

Seu processo de criação acontece nesta ordem:

```text
1. Alocar o Mapa
2. Alocar 20 linhas de células
3. Inicializar as 800 células e seus mutexes
4. Criar as 3 vias horizontais
5. Criar as 4 vias verticais
6. Criar os 12 cruzamentos
```

Na destruição, a ordem é invertida:

```text
cruzamentos → vias → células → Mapa
```

Essa ordem evita usar uma estrutura depois que a memória da qual ela depende já foi liberada.

### `src/models/Cruzamento.h` e `Cruzamento.c`

Representam uma interseção entre uma via horizontal e uma vertical.

Cada cruzamento possui dois sinais:

```text
sinal_h → controla a via horizontal
sinal_v → controla a via vertical
```

Os dois fluxos não devem ficar verdes simultaneamente. O estado inicial é:

```text
horizontal = VERDE
vertical   = VERMELHO
```

Ao alternar:

```text
horizontal = VERMELHO
vertical   = VERDE
```

O cruzamento também possui:

- um mutex para proteger seu estado;
- uma variável de condição para veículos horizontais;
- uma variável de condição para veículos verticais;
- campos reservados para a prioridade da ambulância.

### `src/models/Relogio_global.h` e `Relogio_global.c`

Implementam a fonte de tempo da simulação.

As variáveis principais são:

| Variável | Função |
|---|---|
| `global_tick` | Número do tick atual |
| `simulation_running` | Informa se a simulação deve continuar |
| `clock_mutex` | Protege o relógio |
| `clock_cond` | Acorda quem está esperando o próximo tick |

A thread do relógio repete:

```text
dormir 100 ms
    ↓
travar clock_mutex
    ↓
incrementar global_tick
    ↓
acordar todas as threads que esperam clock_cond
    ↓
destravar clock_mutex
```

No código atual, portanto:

```text
1 tick = aproximadamente 100 milissegundos
```

### `src/models/Semaforo.h` e `Semaforo.c`

Implementam um semáforo de trânsito isolado, usado no teste de `main.c`.

Ele começa vermelho e conta quantos ticks permaneceu no estado atual. No exemplo da `main`, os tempos são:

```text
3 ticks vermelho → 3 ticks verde → repetir
```

### `src/models/Veiculo.h` e `Veiculo.c`

Definem os dados básicos de um veículo:

- ID;
- velocidade;
- via atual;
- índice da posição na via;
- linha e coluna;
- identificador da thread;
- estado ativo;
- ponteiro para o mapa.

A velocidade significa quantos ticks deveriam existir entre os movimentos:

| Velocidade | Interpretação planejada |
|---:|---|
| `1` | Move a cada tick |
| `2` | Move a cada 2 ticks |
| `4` | Move a cada 4 ticks |

Atualmente esse módulo possui apenas `veiculo_init`. A thread de um veículo comum ainda não foi implementada.

### `src/models/Ambulancia.h` e `Ambulancia.c`

Possuem uma função de thread para movimentar uma ambulância.

A ambulância atual:

1. tenta ocupar a célula inicial;
2. espera o próximo tick;
3. calcula a próxima célula;
4. espera o sinal verde se a próxima célula for um cruzamento;
5. tenta ocupar o destino;
6. libera a célula anterior;
7. tem 30% de chance de trocar de via em um cruzamento;
8. volta ao começo da via ao chegar ao fim.

Apesar de a função existir, ela ainda não é iniciada por `main.c`.

Além disso, a prioridade especial ainda não está integrada. O próprio código informa que, por enquanto, a ambulância respeita o sinal como um veículo comum.

### `src/models/Elementos.h`

Contém textos e símbolos para uma futura visualização ASCII:

- ruas;
- cruzamentos;
- semáforos;
- carros;
- ambulância;
- edifícios;
- casas;
- parque;
- exemplos de frames.

Esses símbolos ainda não são usados por `main.c`. Portanto, existe um protótipo visual, mas ainda não existe um renderizador integrado do mapa.

---

## 5. O que é uma thread neste projeto

Uma thread é um fluxo de execução dentro do mesmo programa.

Em uma versão completa do simulador, a ideia é ter algo parecido com:

```text
Programa
├── thread principal
├── thread do relógio
├── thread do veículo 1
├── thread do veículo 2
├── ...
├── thread do veículo N
└── thread da ambulância
```

Todas essas threads enxergam a mesma memória. Isso permite que compartilhem o mapa, mas também cria risco.

Exemplo do problema:

```text
Veículo A verifica: "a célula está livre"
Veículo B verifica: "a célula está livre"
Veículo A ocupa a célula
Veículo B também ocupa a célula
```

Sem sincronização, os dois podem tomar uma decisão usando um estado desatualizado. Isso é uma **condição de corrida**.

---

## 6. Por que cada célula tem um mutex

Um mutex funciona como uma chave que somente uma thread pode possuir por vez.

A operação `celula_tentar_ocupar` faz, de forma simplificada:

```text
travar o mutex da célula
    ↓
a célula está livre?
    ├── sim: marcar como ocupada e guardar o veículo
    └── não: informar falha
    ↓
destravar o mutex
```

Assim, mesmo que dois veículos tentem entrar na mesma célula ao mesmo tempo, a verificação e a ocupação são protegidas pela mesma trava.

O primeiro veículo ocupa. Quando o segundo consegue entrar na seção crítica, ele já encontra `ocupada = 1` e falha.

### Por que não existe um único mutex para o mapa inteiro?

Um único mutex global seria mais simples, mas impediria movimentos independentes.

Com um mutex por célula:

```text
Veículo A pode acessar a célula (4,5)
ao mesmo tempo que
Veículo B acessa a célula (16,30)
```

Como são recursos diferentes, não há necessidade de bloquear o mapa inteiro.

Esse tipo de proteção é chamado de **bloqueio de granularidade fina**.

---

## 7. Para que serve uma variável de condição

O mutex protege dados, mas não é a melhor ferramenta para esperar um evento.

Imagine um veículo no sinal vermelho. Uma solução ruim seria:

```c
while (sinal == VERMELHO) {
    /* verificar novamente sem parar */
}
```

Esse laço consome CPU sem realizar trabalho útil. É chamado de **espera ocupada** ou **busy-wait**.

Uma variável de condição permite que a thread durma:

```text
veículo encontra sinal vermelho
    ↓
pthread_cond_wait(...)
    ↓
a thread dorme e não disputa CPU
    ↓
o semáforo abre e executa pthread_cond_broadcast(...)
    ↓
a thread acorda e verifica novamente o sinal
```

O projeto usa esse mecanismo em dois lugares:

1. `clock_cond`: acorda as threads quando chega um novo tick;
2. `cond_h` e `cond_v`: acordam veículos quando a direção correspondente fica verde.

---

## 8. Como o relógio coordena as threads

Sem o relógio, cada veículo poderia executar na velocidade máxima permitida pelo processador. O resultado dependeria mais da máquina do que das regras da simulação.

O relógio cria uma unidade lógica comum:

```text
Tick 1 → entidades fazem no máximo uma etapa
Tick 2 → entidades fazem a próxima etapa
Tick 3 → entidades fazem a próxima etapa
```

A função `esperar_proximo_tick(tick_atual)` bloqueia até que `global_tick` seja diferente do valor recebido.

Exemplo:

```text
global_tick vale 7
    ↓
a thread chama esperar_proximo_tick(7)
    ↓
a thread dorme
    ↓
o relógio altera global_tick para 8 e faz broadcast
    ↓
a thread acorda
```

O `while` usado ao redor de `pthread_cond_wait` é necessário porque uma thread pode acordar sem que a condição desejada tenha se tornado verdadeira. Ela sempre deve verificar novamente.

---

## 9. Como um movimento é realizado

A única rotina de movimento existente está na thread da ambulância.

Considere que ela está na célula A e quer ir para a célula B:

```text
[ célula A: ambulância ] → [ célula B: livre ]
```

O fluxo atual é:

```text
1. Calcular o índice da próxima célula
2. Se for cruzamento, esperar o sinal verde
3. Tentar ocupar a próxima célula
4. Se conseguir, liberar a célula anterior
5. Atualizar a posição armazenada no veículo
```

Depois:

```text
[ célula A: livre ] → [ célula B: ambulância ]
```

Se B já estiver ocupada, a tentativa falha e a ambulância permanece em A.

### Limitação importante

O movimento “ocupa o destino e depois libera a origem”. Isso evita que o veículo desapareça caso o destino esteja ocupado, mas existe um pequeno intervalo em que ele aparece como ocupante das duas células.

Para uma renderização consistente e para uma política rigorosa de movimento atômico, essa transição ainda precisa de um protocolo único de locks ou de snapshot.

---

## 10. Como os semáforos dos cruzamentos deveriam funcionar

Cada cruzamento tem dois fluxos conflitantes:

```text
Estado A:
horizontal = VERDE
vertical   = VERMELHO

Estado B:
horizontal = VERMELHO
vertical   = VERDE
```

A função `cruzamento_alternar_sinal`:

1. trava o mutex do cruzamento;
2. verifica os campos de prioridade da ambulância;
3. troca os dois sinais;
4. acorda os veículos da direção que ficou verde;
5. libera o mutex.

Por exemplo:

```text
horizontal fecha
    ↓
vertical abre
    ↓
pthread_cond_broadcast(&cond_v)
    ↓
veículos verticais acordam
```

### Existem dois modelos de semáforo no código

Atualmente o projeto possui:

1. `Semaforo`, em `Semaforo.h`;
2. `sinal_h` e `sinal_v`, dentro de `Cruzamento`.

Eles não estão conectados.

`main.c` atualiza somente o primeiro modelo. Já os veículos e a ambulância consultariam o segundo.

Isso significa que, mesmo se a ambulância fosse iniciada agora, o semáforo impresso pela `main` não controlaria os cruzamentos do mapa.

Antes da integração completa, o projeto precisa definir **uma única fonte de verdade** para os sinais.

---

## 11. Como deveria funcionar a prioridade da ambulância

O `Cruzamento` já possui:

```c
int ambulancia_presente;
DirecaoVia direcao_ambulancia;
```

A ideia planejada é:

```text
ambulância se aproxima
    ↓
marca ambulancia_presente = 1
    ↓
informa se está na horizontal ou vertical
    ↓
o cruzamento mantém ou concede verde para essa direção
    ↓
ambulância atravessa
    ↓
remove a solicitação de prioridade
    ↓
o ciclo normal volta a funcionar
```

A função de alternância já evita fechar uma direção que esteja verde para uma ambulância registrada.

Porém, a thread da ambulância ainda não:

- registra sua aproximação;
- força a abertura do sinal quando ele está vermelho;
- remove a prioridade após atravessar.

Logo, a infraestrutura existe parcialmente, mas o protocolo completo ainda não.

---

## 12. O que o programa executa hoje

Esta é a parte mais importante para não confundir código planejado com código integrado.

O fluxo real de `main.c` é:

```text
main
  ↓
init_relogio()
  ↓
init_semaforo(&sem1, id=1, verde=3, vermelho=3)
  ↓
criar thread_relogio
  ↓
repetir 10 vezes:
    esperar próximo tick
    atualizar sem1
    imprimir tick e estado
  ↓
simulation_running = false
  ↓
aguardar a thread do relógio com join
  ↓
destruir semáforo e relógio
```

A saída é semelhante a:

```text
[Tick: 01] Semaforo 1 esta: VERMELHO
[Tick: 02] Semaforo 1 esta: VERMELHO
[Tick: 03] Semaforo 1 esta: VERDE
...
[Tick: 09] Semaforo 1 esta: VERDE
[Tick: 10] Semaforo 1 esta: VERDE
```

### O que ainda não participa dessa execução

| Componente | Existe no código? | É usado pela `main`? |
|---|---:|---:|
| Relógio global | Sim | Sim |
| Semáforo isolado de teste | Sim | Sim |
| Mapa 20 × 40 | Sim | Não |
| 7 vias | Sim | Não |
| 12 cruzamentos | Sim | Não |
| Mutex por célula | Sim | Não durante a execução atual |
| Thread de veículo comum | Não | Não |
| Thread da ambulância | Sim | Não |
| Prioridade da ambulância | Parcial | Não |
| Visualização ASCII | Apenas símbolos | Não |

Portanto, o executável atual é um **protótipo da base temporal e da alternância de um semáforo**, não a simulação urbana completa.

---

## 13. Como a versão completa deveria iniciar e encerrar

O fluxo final esperado é aproximadamente:

```text
1. Inicializar o relógio
2. Criar o mapa
3. Configurar os cruzamentos
4. Criar os veículos
5. Criar a ambulância
6. Iniciar a thread do relógio
7. Iniciar as threads dos veículos
8. Iniciar a thread da ambulância
9. A cada tick:
   - atualizar sinais;
   - permitir movimentos;
   - produzir um frame consistente do mapa.
10. Solicitar o encerramento
11. Acordar threads que estejam bloqueadas
12. Executar join em todas as threads
13. Destruir mutexes e variáveis de condição
14. Liberar toda a memória
```

O encerramento exige atenção especial. Alterar apenas `simulation_running` não basta se alguma thread estiver dormindo indefinidamente em uma variável de condição. O fluxo final deve fazer broadcasts de encerramento antes dos `join`.

---

## 14. Relação entre os conceitos de Sistemas Operacionais e o código

| Conceito | Onde aparece | Para que serve |
|---|---|---|
| Thread | Relógio e ambulância | Executar entidades concorrentemente |
| Memória compartilhada | Mapa e variáveis globais | Permitir que threads observem o mesmo estado |
| Seção crítica | Ocupação de célula e alteração de sinal | Isolar operações que não podem se intercalar |
| Mutex | `Celula`, `Cruzamento`, relógio e semáforo | Garantir acesso exclusivo a dados compartilhados |
| Variável de condição | Relógio e cruzamentos | Esperar eventos sem consumir CPU continuamente |
| Condição de corrida | Possível ao acessar ocupação, tick ou sinais sem proteção | Erro que a sincronização tenta evitar |
| Deadlock | Possível ao adquirir vários locks em ordens diferentes | Situação que a política de locks deve prevenir |
| `join` | Encerramento da thread do relógio | Esperar uma thread terminar antes de liberar recursos |

### Semáforo de trânsito não é semáforo de SO

O projeto fala muito em “semáforo”, mas existem dois significados diferentes:

- **semáforo de trânsito:** luz verde ou vermelha que controla uma via;
- **semáforo de Sistemas Operacionais:** primitiva de sincronização baseada em um contador.

O código atual usa semáforos **de trânsito**, mutexes e variáveis de condição. Ele não usa a primitiva POSIX `sem_t`.

---

## 15. Pontos que ainda precisam ser integrados ou corrigidos

### Integração funcional

- criar e destruir o mapa na `main`;
- implementar a thread dos veículos comuns;
- criar entre 10 e 20 veículos;
- iniciar e aguardar todas as threads;
- ligar os cruzamentos ao relógio;
- renderizar o mapa a cada tick.

### Modelo de trânsito

- representar corretamente as duas faixas de vias de mão dupla;
- aplicar a velocidade dos veículos;
- definir rotas e sentidos válidos;
- decidir como veículos esperam uma célula ocupada;
- impedir ultrapassagem onde a regra exigir.

### Sincronização

- unificar os dois modelos de semáforo de trânsito;
- definir uma operação segura de movimento entre origem e destino;
- proteger consistentemente `global_tick` e `simulation_running`;
- acordar todas as threads durante o encerramento;
- definir uma ordem global de aquisição de locks para evitar deadlock;
- criar uma forma consistente de o renderizador ler o mapa.

### Ambulância

- solicitar prioridade antes do cruzamento;
- receber o sinal verde;
- liberar a prioridade depois da passagem;
- impedir que a prioridade permaneça ativa para sempre.

### Compatibilidade

O relógio e o semáforo isolado possuem código condicional para Linux e Windows. Porém, `Celula`, `Cruzamento`, `Veiculo` e `Ambulancia` usam diretamente tipos e funções de `pthread`.

Assim, a compilação completa está preparada e verificada no Linux, mas a compatibilidade integral com Windows ainda não está implementada.

---

## 16. Como compilar e observar o estado atual

Na raiz do projeto:

```bash
make
./bin/simulador
```

Para limpar os objetos e o executável:

```bash
make clean
```

O `Makefile` compila todos os módulos, mesmo aqueles que ainda não são chamados pela `main`. Por isso, “compilar sem erros” prova que os arquivos são aceitos pelo compilador, mas não prova que toda a simulação está integrada ou funcionando.

---

## 17. Ordem recomendada para estudar o código

Se você está começando agora, leia nesta ordem:

1. `src/main.c` — para entender o que realmente executa;
2. `Relogio_global.h` e `Relogio_global.c` — para entender ticks e espera;
3. `Semaforo.h` e `Semaforo.c` — para entender o teste atual;
4. `Celula.h` e `Celula.c` — para entender exclusão mútua;
5. `Via.h` e `Via.c` — para entender a sequência de posições;
6. `Mapa.h` e `Mapa.c` — para visualizar como tudo é montado;
7. `Cruzamento.h` e `Cruzamento.c` — para entender sinais e condições;
8. `Veiculo.h` e `Veiculo.c` — para entender os dados de um veículo;
9. `Ambulancia.h` e `Ambulancia.c` — para acompanhar um movimento completo;
10. `Elementos.h` — para conhecer a visualização planejada.

Ao ler cada módulo, faça três perguntas:

```text
1. Quem cria esta estrutura?
2. Qual thread pode alterar seus dados?
3. Qual mutex protege essa alteração?
```

Se uma dessas respostas não estiver clara, provavelmente existe um contrato de integração ainda não definido.

---

## 18. Resumo final

O projeto possui boa parte das estruturas básicas da cidade:

- mapa;
- células protegidas por mutex;
- vias;
- cruzamentos;
- relógio global;
- semáforos;
- modelo de veículo;
- uma rotina inicial de ambulância;
- símbolos ASCII.

Mas essas partes ainda estão em estágios diferentes de implementação.

Hoje, o programa principal demonstra apenas:

```text
thread do relógio + espera por condição + alternância de um semáforo
```

O objetivo da integração é chegar a:

```text
relógio
   ↓ acorda
veículos concorrentes
   ↓ consultam
semáforos dos cruzamentos
   ↓ protegem
movimentos entre células
   ↓ aparecem em
visualização ASCII consistente
```

Em termos de Sistemas Operacionais, o desafio central é garantir que todas essas threads avancem juntas sem:

- dois veículos ocuparem a mesma célula;
- um veículo atravessar o sinal vermelho;
- uma thread consumir CPU apenas esperando;
- duas threads ficarem bloqueadas para sempre;
- o programa liberar memória antes de as threads terminarem.

---

## Documentos relacionados

- [Conceitos de Sistemas Operacionais](conceitos_so.md) — threads, mutexes, condições de corrida e deadlocks
- [Memória Virtual](memoria_virtual.md) — paginação, MMU, page faults e swap
- [Divisão de tarefas](tasks-division.md) — estado de implementação, responsabilidades e próximos passos
