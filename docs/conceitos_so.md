# 📚 Conceitos Básicos de Sistemas Operacionais

> Guia de consulta rápida para a disciplina de **Sistemas Operacionais**.  
> Cada tópico inclui um link direto para a aula correspondente na playlist [Unix Threads in C — CodeVault](https://www.youtube.com/playlist?list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2).

---

## 1. Processo vs. Thread

| | **Processo** | **Thread** |
|---|---|---|
| **O que é** | Programa em execução | "Processo leve" que roda *dentro* de um processo |
| **Memória** | Espaço de endereçamento **isolado** | **Compartilha** a memória global do processo pai |
| **Criação** | Pesada (cópia de memória via `fork()`) | Leve e rápida (`pthread_create()`) |
| **Comunicação** | Requer IPC (pipes, sockets, etc.) | Direta (lê/escreve as mesmas variáveis) |
| **Risco** | Baixo (isolados entre si) | Alto — exige Mutex e sincronização para evitar *Race Conditions* |

> ⚠️ **Tradeoff das Threads:** A alteração de uma variável global na `thread X` reflete instantaneamente na `thread Y`. Isso agiliza a comunicação, mas exige extremo cuidado.

🔗 [▶️ Assistir: Difference between processes and threads](https://www.youtube.com/watch?v=uA8X5zNOGw8&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 2. Criação de Processos — Fork()

A chamada de sistema `fork()` **clona** o processo atual (Pai), criando um **Processo Filho** idêntico.

- **Cópia estrita:** Todas as variáveis globais são duplicadas em espaços de memória separados.
- **Isolamento total:** Alterar uma variável no filho **não afeta** o pai (ao contrário das Threads).
- **Retorno:** `0` no processo filho | `PID` do filho no processo pai | `-1` em caso de erro.

```c
pid_t pid = fork();
if (pid == 0)      { /* Código do Filho */ }
else if (pid > 0)  { /* Código do Pai   */ }
```

🔗 [▶️ Assistir: fork() in C](https://www.youtube.com/watch?v=cex9XrZCU14&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 3. Concorrência vs. Paralelismo

| | **Concorrência** | **Paralelismo** |
|---|---|---|
| **Definição** | Várias tarefas fazendo progresso **intercalando** o uso da CPU | Várias tarefas rodando **literalmente ao mesmo tempo** |
| **Requisito** | 1 núcleo (basta troca de contexto) | Múltiplos núcleos físicos |
| **Analogia** | 1 cozinheiro alternando entre pratos | 4 cozinheiros, cada um fazendo um prato |

🔗 [▶️ Assistir: Short introduction to threads (pthreads)](https://www.youtube.com/watch?v=d9s_d28yJq0&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 4. Condição de Corrida (Race Condition)

Comportamento **imprevisível** que ocorre quando múltiplas threads leem e alteram a mesma variável compartilhada simultaneamente. O resultado depende da ordem de quem agiu primeiro.

> *Exemplo no simulador:* Dois carros verificam que o mesmo cruzamento está vazio e tentam entrar nele no exato mesmo instante → "colisão" na memória.

🔗 [▶️ Assistir: What are Race Conditions?](https://www.youtube.com/watch?v=FY9livorrJI&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 5. Seção Crítica (Critical Section)

O trecho de código que manipula **dados compartilhados**. Somente **uma thread** deve executá-lo por vez.

```
Thread A ──► [LOCK] ──► Seção Crítica ──► [UNLOCK]
Thread B ──► espera...  ──────────────► [LOCK] ──► Seção Crítica ──► [UNLOCK]
```

🔗 [▶️ Assistir: What is a mutex in C? (pthread_mutex)](https://www.youtube.com/watch?v=oq29KUy29iQ&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 6. Exclusão Mútua (Mutex)

O "**cadeado**" que protege a seção crítica:

| Operação | Função em C | O que faz |
|---|---|---|
| Trancar | `pthread_mutex_lock()` | A thread entra na seção crítica; as outras ficam bloqueadas |
| Destrancar | `pthread_mutex_unlock()` | Libera o cadeado; a próxima thread bloqueada é acordada |

> *No simulador:* Cada célula do mapa tem um mutex próprio. Um carro trava o mutex antes de ocupar a célula e destranca ao sair.

🔗 [▶️ Assistir: What is a mutex in C? (pthread_mutex)](https://www.youtube.com/watch?v=oq29KUy29iQ&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 7. Variáveis de Condição (Condition Variables)

Ferramenta para uma thread **dormir** até que uma condição específica ocorra, sem consumir CPU.

| Operação | Função em C | O que faz |
|---|---|---|
| Esperar | `pthread_cond_wait()` | A thread dorme e libera o mutex automaticamente |
| Acordar 1 | `pthread_cond_signal()` | Acorda **uma** thread que está esperando |
| Acordar todas | `pthread_cond_broadcast()` | Acorda **todas** as threads que estão esperando |

> *No simulador:* O relógio global usa `pthread_cond_broadcast()` para acordar todos os veículos a cada tick.

🔗 [▶️ Assistir: Condition variables in C](https://www.youtube.com/watch?v=0sGlPe6heqA&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 8. Semáforos (Sistemas Operacionais)

> ⚠️ Não confundir com os semáforos de **trânsito** do simulador!

O semáforo do SO é uma **variável contadora** que controla o acesso a recursos com capacidade limitada.

| Operação | Nome Clássico | O que faz |
|---|---|---|
| `sem_wait()` | P (Proberen) | Diminui o contador. Se chegar a 0, a thread **espera** |
| `sem_post()` | V (Verhogen) | Aumenta o contador e **acorda** quem estiver esperando |

🔗 [▶️ Assistir: Introduction to semaphores in C](https://www.youtube.com/watch?v=ukM_zzrIeXs&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 9. Deadlock (Impasse)

Travamento **definitivo** onde duas ou mais threads ficam eternamente bloqueadas, cada uma esperando que a outra libere um recurso.

**4 condições necessárias (Coffman):**
1. **Exclusão mútua** — o recurso não pode ser compartilhado.
2. **Posse e espera** — a thread segura um recurso enquanto espera outro.
3. **Não preempção** — ninguém pode forçar a thread a soltar o recurso.
4. **Espera circular** — A espera B, B espera C, C espera A.

> *Exemplo no simulador:* Carro A reserva a Via X e precisa da Via Y. Carro B reserva a Via Y e precisa da Via X. Nenhum solta o que tem → impasse eterno.

🔗 [▶️ Assistir: Deadlocks in C](https://www.youtube.com/watch?v=_1B1H7T2Ghs&list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

---

## 📎 Documentos Relacionados

- [Explicação do Projeto](file:///home/alan/Documentos/simulador-de-trafico-urbano/docs/explicacao_projeto.md) — Como o simulador funciona na prática
- [Memória Virtual](file:///home/alan/Documentos/simulador-de-trafico-urbano/docs/memoria_virtual.md) — Paginação, MMU, Page Faults e Swap
