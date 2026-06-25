# 🚦 Simulador de Tráfego Urbano em C — Plano de Execução

> **Equipe:** Alan · Diogo · Neto · Cícero
>
> **Janela revisada:** 23/06/2026 → 03/07/2026 (11 dias corridos)
>
> **Última atualização:** 24/06/2026
>
> **Disciplina:** Sistemas Operacionais — Concorrência, Sincronização e Deadlocks

---

## Objetivo da entrega

Entregar uma simulação concorrente de tráfego urbano em C na qual:

- cada veículo, incluindo a ambulância, execute em sua própria thread;
- o relógio global coordene o avanço da simulação por ticks;
- veículos respeitem vias, semáforos e limites do mapa;
- nenhuma célula seja ocupada por mais de um veículo;
- esperas usem mecanismos bloqueantes, sem espera ocupada;
- a ambulância receba prioridade sem violar a segurança;
- a execução seja observável por uma visualização ASCII no terminal.

## Definição de pronto

Uma tarefa só pode ser marcada como concluída quando:

1. o código compila sem erros com `make`;
2. o comportamento foi integrado ao fluxo principal, não apenas implementado isoladamente;
3. existe uma forma objetiva de validar o resultado;
4. os recursos criados — threads, mutexes, condições e memória — são encerrados corretamente;
5. a alteração foi revisada por pelo menos outro integrante.

### Legenda

- [x] Concluído e presente no repositório
- [ ] Pendente ou ainda sem validação integrada
- **P0:** bloqueia a simulação mínima
- **P1:** necessário para a entrega completa
- **P2:** acabamento, documentação ou melhoria

---

## Estado atual verificado — 24/06

| Área | Estado | Evidência atual | Próximo passo |
|---|---|---|---|
| Estruturas e mapa | Parcialmente integrado | `Celula`, `Via`, `Cruzamento` e `Mapa` existem; a malha possui 12 cruzamentos | Inicializar o mapa na `main` e validar vias de mão dupla |
| Exclusão mútua | Implementada na base | Cada `Celula` possui mutex e operações de ocupação/liberação | Integrar ao movimento e testar disputa pela mesma célula |
| Relógio global | Protótipo funcional | Thread, tick e variável de condição existem | Integrar veículos, cruzamentos e encerramento global |
| Semáforos | Parcialmente implementados | Há estados em `Semaforo` e também em `Cruzamento` | Escolher um único modelo e remover duplicidade de responsabilidade |
| Veículos | Não iniciado | Não há modelo nem thread de veículo | Implementar `Veiculo` e seu ciclo de vida |
| Ambulância | Apenas suporte estrutural | `Cruzamento` possui campos de prioridade; símbolos ASCII existem | Implementar thread, solicitação e liberação da prioridade |
| Visualização | Protótipo de símbolos | Elementos ASCII estão definidos em `Elementos.h` | Criar renderizador do mapa atualizado a cada tick |
| Testes | Não iniciado | Não há suíte ou roteiro de testes | Criar testes de integração e checklist de execução |
| Documentação | Em andamento | `README.md` já possui compilação e execução | Atualizar arquitetura, regras e limitações após a integração |

---

## Responsabilidades e tarefas

### 🔵 Alan — Mapa, exclusão mútua e integração

**Resultado esperado:** fornecer a infraestrutura compartilhada e garantir que todos os módulos operem sobre o mesmo estado da simulação.

- [x] **P0** Criar o repositório e a estrutura inicial do projeto.
- [x] **P0** Implementar `Celula`, `Via`, `Cruzamento` e `Mapa`.
- [x] **P0** Criar malha com pelo menos oito cruzamentos e uma via de mão única.
- [x] **P0** Criar mutex por célula e operações atômicas de ocupação/liberação.
- [ ] **P0** Inicializar e destruir o mapa no fluxo principal.
- [ ] **P0** Definir com Diogo a API única de movimento entre duas células.
- [ ] **P0** Implementar e documentar a estratégia anti-deadlock.
- [ ] **P1** Corrigir ou validar a representação das duas faixas em vias de mão dupla.
- [ ] **P1** Coordenar a integração dos módulos sem sobrescrever mudanças de outras branches.
- [ ] **P2** Finalizar o `README.md` com arquitetura, compilação, execução e limitações.

**Critérios de aceite**

- Duas threads não conseguem ocupar a mesma célula.
- Uma tentativa de movimento nunca deixa o veículo em duas células.
- O mapa é criado e destruído sem vazamentos ou mutexes pendentes.
- A política de aquisição de recursos é única e conhecida por toda a equipe.

### 🟢 Diogo — Veículos, rotas e movimento

**Resultado esperado:** executar entre 10 e 20 veículos simultâneos, com movimento válido e bloqueante.

- [ ] **P0** Definir `Veiculo` com ID, tipo, posição, direção, velocidade, rota e thread.
- [ ] **P0** Implementar criação, execução, encerramento e `join` das threads.
- [ ] **P0** Implementar movimento apenas para células adjacentes e válidas.
- [ ] **P0** Integrar movimento às operações de ocupação/liberação de `Celula`.
- [ ] **P0** Bloquear sem consumir CPU ao aguardar célula ou sinal.
- [ ] **P1** Respeitar sentido da via, faixa e limites do mapa.
- [ ] **P1** Implementar velocidades: rápido = 1 tick, médio = 2 ticks e lento = 4 ticks.
- [ ] **P1** Criar entre 10 e 20 veículos com rotas reproduzíveis.
- [ ] **P1** Impedir ultrapassagem em vias de mão única.
- [ ] **P2** Produzir logs mínimos de criação, bloqueio, movimento e encerramento.

**Critérios de aceite**

- Nenhum veículo se teletransporta, sai do mapa ou ignora o sentido da via.
- O veículo libera a célula anterior somente como parte de uma transição segura.
- A diferença entre velocidades é observável pelo número de ticks.
- Todos os veículos encerram e são aguardados pela thread principal.

### 🔴 Neto — Relógio, sinais e sincronização

**Resultado esperado:** manter uma única fonte de tempo e controlar cruzamentos sem corrida de dados ou espera ocupada.

- [x] **P0** Implementar a thread do relógio global.
- [x] **P0** Acordar threads a cada tick com variável de condição.
- [x] **P0** Implementar um protótipo de alternância verde/vermelho.
- [ ] **P0** Unificar o estado dos sinais entre `Semaforo` e `Cruzamento`.
- [ ] **P0** Integrar a alternância de todos os cruzamentos ao relógio global.
- [ ] **P0** Garantir espera bloqueante no vermelho com `pthread_cond_wait`.
- [ ] **P0** Implementar transição segura, sem liberar fluxos conflitantes.
- [ ] **P1** Definir o protocolo de prioridade usado pela ambulância.
- [ ] **P1** Validar proteção de leitura e escrita do tick e dos sinais.
- [ ] **P1** Garantir encerramento sem threads presas em variáveis de condição.

**Critérios de aceite**

- Existe apenas uma fonte de verdade para o estado de cada sinal.
- Fluxos horizontal e vertical nunca ficam verdes ao mesmo tempo.
- Threads bloqueadas são acordadas quando o sinal abre ou a simulação termina.
- A execução termina sem deadlock e sem acesso concorrente desprotegido.

### 🟡 Cícero — Ambulância, visualização e relatório

**Resultado esperado:** tornar o estado concorrente observável e demonstrar prioridade segura da ambulância.

- [x] **P1** Definir símbolos distintos para vias, cruzamentos, sinais, carros e ambulância.
- [ ] **P0** Implementar a thread da ambulância usando a mesma API dos demais veículos.
- [ ] **P0** Solicitar prioridade antes de entrar em um cruzamento.
- [ ] **P0** Liberar a prioridade imediatamente após a passagem.
- [ ] **P0** Garantir que a prioridade não viole a ocupação exclusiva de células.
- [ ] **P1** Registrar solicitação, concessão e liberação da prioridade.
- [ ] **P1** Implementar renderização ASCII do mapa completo.
- [ ] **P1** Atualizar a tela a cada tick com saída sincronizada.
- [ ] **P1** Evitar que logs concorrentes corrompam a visualização.
- [ ] **P2** Escrever o relatório final com arquitetura, sincronização, testes e anti-deadlock.

**Critérios de aceite**

- A ambulância obtém sinal verde de modo seguro e não atravessa célula ocupada.
- A prioridade não deixa o cruzamento permanentemente bloqueado.
- Cada frame representa um estado consistente da simulação.
- O relatório descreve o comportamento realmente implementado.

---

## Contratos de integração

Estas decisões devem ser fechadas antes da integração parcial de 26/06:

| Contrato | Responsáveis | Decisão necessária |
|---|---|---|
| Movimento entre células | Alan + Diogo | Ordem de locks, condição de espera e atualização atômica da posição |
| Tick global | Diogo + Neto | Como cada veículo detecta e consome exatamente o próximo tick |
| Estado do cruzamento | Alan + Neto | Estrutura proprietária dos sinais e mutex que protege o estado |
| Prioridade da ambulância | Neto + Cícero | Solicitação, confirmação, passagem e liberação |
| Snapshot para renderização | Alan + Cícero | Como ler mapa e veículos sem exibir estado parcialmente atualizado |
| Encerramento | Todos | Flag global, broadcasts finais, `join` e destruição dos recursos |

### Estratégia anti-deadlock proposta

Adotar uma regra única: a thread nunca deve aguardar sinal ou tick enquanto mantém o mutex de uma célula. Quando precisar adquirir mais de um recurso, deve usar uma ordem global e determinística — por exemplo, o índice linear `linha * MAPA_COLUNAS + coluna`.

Antes de implementar outra estratégia, Alan e Diogo devem confirmar que o protocolo de movimento preserva simultaneamente:

1. ocupação exclusiva;
2. ausência de espera circular;
3. ausência de espera ocupada;
4. impossibilidade de o veículo desaparecer ou ocupar duas células.

---

## Cronograma de entrega

### Fase 1 — Caminho crítico (23/06 → 26/06)

| Data | Marco verificável |
|---|---|
| **Ter 23/06** | Estruturas, mapa, relógio e símbolos-base no repositório |
| **Qua 24/06** | API de veículo definida; mapa inicializado; modelo único de semáforo decidido |
| **Qui 25/06** | Um veículo percorre rota, espera sinal e disputa células sem busy-wait |
| **Sex 26/06** | Integração parcial: múltiplos veículos + relógio + mapa + sinais |

### Fase 2 — Funcionalidades completas (27/06 → 30/06)

| Data | Marco verificável |
|---|---|
| **Sáb–Dom 27–28/06** | 10–20 veículos, velocidades, ambulância e visualização por tick |
| **Seg 29/06** | Testes de concorrência, correção de deadlocks e revisão conjunta |
| **Ter 30/06** | Código funcional congelado para testes finais; README e relatório em revisão |

### Fase 3 — Validação e entrega (01/07 → 03/07)

| Data | Marco verificável |
|---|---|
| **Qua 01/07** | Regressão completa e execução prolongada sem travamentos |
| **Qui 02/07** | Freeze: somente correções críticas e documentação |
| **Sex 03/07** | Entrega do código, relatório e histórico do repositório |

---

## Plano mínimo de testes

| Cenário | Resultado esperado | Responsáveis |
|---|---|---|
| Dois veículos tentam ocupar a mesma célula | Apenas um avança; o outro bloqueia sem busy-wait | Alan + Diogo |
| Veículo encontra sinal vermelho | Aguarda e avança somente após o verde | Diogo + Neto |
| Fluxos opostos chegam ao cruzamento | Não há ocupação simultânea nem sinais conflitantes | Alan + Neto |
| Ambulância solicita prioridade | Sinal muda de forma segura e volta ao ciclo normal | Neto + Cícero |
| 10–20 veículos em execução prolongada | Sem deadlock, corrida visível ou saída do mapa | Todos |
| Encerramento durante espera | Todas as threads acordam, terminam e recebem `join` | Todos |
| Renderização concorrente | Frame sem linhas intercaladas ou estado inconsistente | Cícero + Alan |

Ferramentas recomendadas para validação:

- compilação com avisos: `-Wall -Wextra -Wpedantic`;
- sanitizadores quando disponíveis: `-fsanitize=address,undefined`;
- análise de concorrência no Linux: ThreadSanitizer ou Valgrind/Helgrind;
- execuções repetidas e prolongadas, não apenas uma demonstração curta.

---

## Entregáveis

| Entregável | Responsável principal | Critério de conclusão |
|---|---|---|
| Código-fonte em C | Todos | Compila e executa a simulação completa |
| `README.md` | Alan | Instalação, compilação, execução, arquitetura e limitações |
| Relatório de implementação | Cícero | Decisões reais, testes, resultados e divisão do trabalho |
| Histórico Git | Alan | Commits identificáveis e integração preservada |
| Divisão de responsabilidades | Cícero | Coerente com commits e relatório final |

## Critérios de avaliação

| Critério | Pontos | Responsável principal |
|---|---:|---|
| Corretude da simulação | 2,0 | Diogo + Alan |
| Visualização ASCII | 2,0 | Cícero |
| Exclusão mútua | 1,5 | Alan |
| Sincronização de sinais | 1,5 | Neto |
| Ambulância com prioridade | 1,0 | Cícero + Neto |
| Ausência de deadlock | 2,0 | Alan + Neto |
| **Total** | **10,0** | **Todos** |

---

## Regras de trabalho

1. Nenhuma espera ocupada: usar variáveis de condição, mutexes ou semáforos bloqueantes.
2. Cada branch deve tratar um escopo pequeno e integrável.
3. Antes do merge, executar `make`, uma simulação curta e o cenário afetado.
4. Mudanças em estruturas compartilhadas devem ser comunicadas aos consumidores do módulo.
5. Commits devem indicar claramente a alteração, sem misturar refatoração não relacionada.
6. Problemas de integração têm prioridade sobre novas funcionalidades após 29/06.
7. O relatório e o README devem descrever o código entregue, não uma arquitetura planejada.
