# 🚦 Simulador de Tráfego Urbano em C — Divisão de Tarefas

> **Equipe:** Alan · Diogo · Neto · Cícero  
> **Prazo:** 16/06/2026 → 03/07/2026 (17 dias)  
> **Disciplina:** Sistemas Operacionais — Concorrência, Sincronização e Deadlocks

---

## 📌 Visão Geral do Projeto

O projeto consiste em simular tráfego urbano concorrente em C com Pthreads. Veículos são **threads** que competem por células de uma malha viária, sincronizadas por **mutexes**, **semáforos** e **variáveis de condição**. A saída é uma visualização ASCII no terminal.

---

## 👥 Responsabilidades por Integrante

### 🔵 Alan — Infraestrutura do Mapa e Arquitetura Central

> Responsável por montar a base do projeto: estruturas de dados, o mapa urbano e a integração entre módulos.

**Tarefas:**
- [ ] Criar o repositório GitHub e configurar a estrutura de pastas (`src/`, `include/`, `docs/`)
- [ ] Definir as **estruturas de dados** principais (`Celula`, `Cruzamento`, `Via`, `Mapa`)
- [ ] Implementar a **malha viária** (matriz com ≥8 cruzamentos, vias horizontais/verticais e ao menos 1 via de mão única)
- [ ] Garantir que o mapa suporte vias de mão dupla com controle de faixa
- [ ] Implementar os **mutexes por célula** (impenetrabilidade — lei 4.1)
- [ ] Implementar a **estratégia anti-deadlock** (ex.: ordem fixa de aquisição de recursos)
- [ ] Escrever o `README.md` com instruções de compilação e execução
- [ ] Fazer o merge final das branches e revisar o código

---

### 🟢 Diogo — Threads de Veículos e Lógica de Movimento

> Responsável pela lógica de cada veículo como thread, seu ciclo de vida, rota e regras de movimento.

**Tarefas:**
- [ ] Implementar a **thread de cada veículo** (`pthread_create`, `pthread_join`)
- [ ] Definir a estrutura `Veiculo` (id, posição, direção, velocidade, rota)
- [ ] Implementar o **motor de movimento**: células adjacentes válidas, sem teletransporte, sem ultrapassagem em mão única
- [ ] Implementar as **3 velocidades** (rápido=1 tick, médio=2 ticks, lento=4 ticks)
- [ ] Gerar entre **10 e 20 carros** rodando simultaneamente
- [ ] Garantir que veículos respeitem a direção da via e os limites do mapa
- [ ] Implementar **bloqueio correto** ao aguardar célula livre (sem espera ocupada)

---

### 🔴 Neto — Relógio Global, Semáforos de Trânsito e Sincronização

> Responsável pelo coração da sincronização: o relógio global que coordena os ticks e os semáforos de cruzamento.

**Tarefas:**
- [ ] Implementar a **thread do relógio global** (coordena ticks da simulação)
- [ ] Implementar **variáveis de condição** para acordar threads a cada tick (sem busy-wait)
- [ ] Implementar os **semáforos de trânsito** em cada cruzamento (verde/vermelho por via)
- [ ] Garantir que carros **bloqueiem sem consumir CPU** no sinal vermelho (`pthread_cond_wait`)
- [ ] Implementar a **transição segura de sinais** (nenhum carro atravessa durante mudança de estado)
- [ ] Implementar **semáforos POSIX** onde necessário (controle de capacidade de cruzamentos)
- [ ] Testar e validar ausência de corrida de dados nos estados de sinal

---

### 🟡 Cícero — Ambulância, Visualização ASCII e Relatório

> Responsável pela feature de prioridade da ambulância, pela interface visual no terminal e pela documentação final.

**Tarefas:**
- [ ] Implementar a **thread da ambulância** com lógica de prioridade em cruzamentos
- [ ] Implementar o sistema de **solicitação de passagem prioritária** (tornar verde a direção necessária quando seguro)
- [ ] Garantir que a prioridade **não viole** a impenetrabilidade de células
- [ ] Registrar visualmente/em log quando a ambulância solicitar prioridade
- [ ] Implementar a **visualização ASCII** no terminal: ruas, cruzamentos, semáforos e veículos identificáveis
- [ ] Atualizar a tela a **cada tick** usando `ncurses` ou sequências ANSI de escape
- [ ] Exibir símbolos distintos para: carro normal, ambulância, sinal verde/vermelho, cruzamento
- [ ] Escrever o **relatório final** (decisões de implementação: mapa, threads, sincronização, anti-deadlock)

---

## 🗓️ Cronograma — 17 Dias (16/06 → 03/07)

### Semana 1 (16/06 → 22/06) — Fundação

| Dia | Alan | Diogo | Neto | Cícero |
|-----|------|-------|------|--------|
| **Seg 16** | Repo GitHub + estruturas de dados | Estrutura `Veiculo` + rascunho de movimento | Rascunho do relógio global | Protótipo visual ASCII (mapa estático) |
| **Ter/Qua 17–18** | Mapa completo (matriz, cruzamentos, vias) | Motor de movimento + velocidades | Variáveis de condição + tick | Símbolos ASCII definitivos |
| **Qui/Sex 19–20** | Mutexes por célula | Geração de 10–20 threads de veículos | Semáforos de trânsito (verde/vermelho) | Thread da ambulância |
| **Sáb/Dom 21–22** | **Integração parcial + revisão conjunta** |||

### Semana 2 (23/06 → 29/06) — Integração e Testes

| Dia | Alan | Diogo | Neto | Cícero |
|-----|------|-------|------|--------|
| **Seg 23** | Implementar estratégia anti-deadlock | Ajustar bloqueio sem busy-wait | Transição segura de sinais | Prioridade da ambulância integrada |
| **Ter/Qua 24–25** | Merge das branches + testes de integração | Testes de movimento e corrida de dados | Testes de sincronização | Atualização de tela por tick |
| **Qui 26** | **Revisão geral do código** | Correção de bugs | Correção de bugs | Registro de log da ambulância |
| **Sex 27** | README.md finalizado | Testes finais | Testes finais | Relatório final |
| **Sáb/Dom 28–29** | **Buffer de emergência / polimento final** |||

### Dias Finais (30/06 → 03/07) — Finalização

| Dia | Atividade |
|-----|-----------|
| **Seg 30** | Revisão final do código e do relatório por toda a equipe |
| **Ter 01/07** | Testes de regressão + ajustes de última hora |
| **Qua 02/07** | Freeze do código — apenas documentação pode ser alterada |
| **Qui 03/07** | 🎯 **ENTREGA**  |

---

## 📦 Entregáveis (responsável principal)

| Entregável | Responsável |
|---|---|
| Código-fonte em C | Todos |
| `README.md` | Alan |
| Relatório de implementação | Cícero |
| Repositório Git com histórico de commits | Alan |
| Lista de integrantes e responsabilidades | Cícero |

---

## 🔗 Critérios de Avaliação vs. Responsável

| Critério | Pontos | Principal Responsável |
|---|---|---|
| Corretude da simulação | 2,0 | Diogo + Alan |
| Visualização ASCII | 2,0 | Cícero |
| Exclusão mútua (mutex) | 1,5 | Alan |
| Sincronização de sinais | 1,5 | Neto |
| Ambulância com prioridade | 1,0 | Cícero |
| Ausência de deadlock | 2,0 | Alan + Neto |
| **Total** | **10,0** | |

---

## ⚠️ Regras Importantes (para todos)

> [!IMPORTANT]
> - **Zero espera ocupada** (`busy-wait`): carros parados em sinal vermelho ou aguardando célula livre DEVEM usar `pthread_cond_wait` ou semáforos bloqueantes.
> - **Commits identificados**: cada commit deve deixar claro quem trabalhou no quê.
> - **Linguagem**: exclusivamente C com Pthreads. Sem C++.

> [!TIP]
> Realizem **reuniões rápidas** ao fim de cada dia para sincronizar o progresso, especialmente na integração entre o relógio (Neto), o movimento (Diogo) e o mapa (Alan).
