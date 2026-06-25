# 🧠 Memória Virtual

> Conceito fundamental de Sistemas Operacionais que permite a cada processo "enxergar" uma memória gigantesca e exclusiva, independentemente da RAM física instalada.

---

## O que é Memória Virtual?

Nos primeiros computadores, programas acessavam a RAM física diretamente. Se a soma dos programas abertos excedesse a memória disponível, o sistema travava.

A **Memória Virtual** resolve isso: o SO, em conjunto com o hardware (processador), dá a cada processo a **ilusão** de possuir um espaço de memória contíguo e exclusivo. Na prática, apenas as partes necessárias do programa ficam na RAM a cada instante.

---

## Endereço Virtual vs. Endereço Físico

| | Endereço Virtual (Lógico) | Endereço Físico |
|---|---|---|
| **Quem usa** | O programa em C (ponteiros, variáveis) | A pastilha de hardware da RAM |
| **Quem traduz** | A **MMU** (Memory Management Unit) no processador | — |
| **Visibilidade** | Cada processo tem o seu próprio espaço | Compartilhado pelo SO entre todos os processos |

> Quando seu código acessa `*ptr = 42`, o endereço em `ptr` é **virtual**. A MMU intercepta e o traduz para o endereço **físico** real na RAM.

---

## Paginação (Paging)

Para tornar a tradução eficiente, a memória é dividida em blocos de tamanho fixo (geralmente **4 KB**):

| Conceito | Espaço | Descrição |
|---|---|---|
| **Página (Page)** | Virtual | Bloco de 4 KB no espaço do processo |
| **Moldura / Quadro (Frame)** | Físico | Bloco de 4 KB na RAM real |

O SO mantém uma **Tabela de Páginas (Page Table)** por processo:

```
┌──────────────────────────────────────────────────┐
│            TABELA DE PÁGINAS (Processo A)         │
├──────────────┬───────────────────────────────────┤
│ Página Virt. │ Moldura Física (Frame)            │
├──────────────┼───────────────────────────────────┤
│      0       │  Frame 37                         │
│      1       │  Frame 102                        │
│      2       │  ❌ Não está na RAM (em disco)    │
│      3       │  Frame 5                          │
└──────────────┴───────────────────────────────────┘
```

---

## Falta de Página (Page Fault)

O SO não carrega o programa inteiro na RAM de uma vez (**Demand Paging** — carregamento sob demanda). Quando a CPU tenta acessar uma página que **não está** mapeada na RAM:

```
1. O hardware emite uma exceção → PAGE FAULT
2. O SO pausa o processo
3. O SO lê os 4 KB do disco (HD/SSD) e os copia para um Frame vazio na RAM
4. O SO atualiza a Tabela de Páginas
5. O processo retoma a execução e acessa o dado normalmente
```

> O processo **não percebe** que foi pausado. Para ele, o acesso à memória foi instantâneo.

---

## Swapping (Troca)

**E se a RAM encher?** O SO utiliza a técnica de **Swapping**:

1. Escolhe uma página na RAM que pertence a um programa ocioso
2. Salva o conteúdo dela no disco:
   - 🐧 **Linux:** partição **Swap**
   - 🪟 **Windows:** arquivo `pagefile.sys`
3. Libera a moldura na RAM para o processo que precisa dela agora

> *Analogia:* Uma mesa de trabalho (RAM) pequena. Você guarda as pastas que não está usando na gaveta (disco) e só coloca na mesa as que precisa no momento.

---

## Por que a Memória Virtual é Revolucionária?

| Vantagem | Explicação |
|---|---|
| **Programas maiores que a RAM** | Permite rodar um jogo de 100 GB com apenas 16 GB de RAM — o SSD serve como "extensão" |
| **Segurança e Isolamento** | Processo A não consegue acessar a memória do Processo B. Cada um tem sua própria Tabela de Páginas. Tentativas ilegais resultam em `Segmentation Fault` |
| **Fim da Fragmentação Externa** | Para o programador, um `array` de 1 milhão de posições parece contínuo na memória. Na RAM física, os blocos podem estar totalmente espalhados — a Tabela de Páginas "costura" tudo |
| **Compartilhamento eficiente** | Bibliotecas como a `libc` podem ser mapeadas em modo somente-leitura na memória de múltiplos processos, usando os mesmos Frames físicos sem duplicação |

---

## 📎 Documentos Relacionados

- [Conceitos de SO](file:///home/alan/Documentos/simulador-de-trafico-urbano/docs/conceitos_so.md) — Threads, Mutex, Deadlock e outros conceitos da disciplina
- [Explicação do Projeto](file:///home/alan/Documentos/simulador-de-trafico-urbano/docs/explicacao_projeto.md) — Como o simulador aplica esses conceitos na prática
