# Simulador de Tráfego Urbano

Primeiro trabalho avaliativo para a disciplina de Sistemas Operacionais.

## 🚦 Sobre o Projeto

Este projeto é um simulador concorrente de tráfego urbano escrito em linguagem C. O objetivo é simular diversas entidades do trânsito (veículos, relógio global e semáforos) interagindo de forma simultânea utilizando **Threads** e técnicas de exclusão mútua/sincronização de recursos (`Mutexes` e variadas condições).

O projeto é modularizado em partes:
- **Base (Relógio e Semáforos):** Cria uma malha de controle do tempo (ticks) com threads operando os semáforos isoladamente.
- **Movimento e Estruturas:** Controle da malha da cidade (vias, cruzamentos) e o tráfego em si por meio das threads que reprensentarão os veículos.

## ⚙️ Como Compilar e Rodar

Este projeto foi projetado para compilar nativamente de forma híbrida: tanto em Linux (usando `pthreads`) quanto em Windows (usando `windows.h`). O projeto utiliza múltiplos arquivos, o que fará a extensão (botão play ou Compile Run) do VS Code falhar dizendo "undefined reference". Deve-se usar o Makefile gerado. 

### 1. Requisitos
Você precisa do compilador GCC instalado (`mingw` no Windows, ou `build-essential` no Linux).

### 2. Compilando o Projeto

Abra um **novo terminal na pasta RAIZ do projeto** e compile usando o `Makefile` digitando:

**Se usar Linux ou WSL**:
```bash
make
```

**Se usar Windows (MinGW)**:
```cmd
mingw32-make
```
*(Isso verificará todos os códigos .c na pasta `/src` e gerará seu executável dentro da pasta oculta transparente `/bin`)*

### 3. Rodando o simulador

Basta rodar o executável que foi embutido na pasta `/bin`.

**Linux / macOS:**
```bash
./bin/simulador
```

**Windows:**
```cmd
.\bin\simulador.exe
```
