# Compilador e Flags
CC = gcc
CFLAGS = -Wall -Wextra -g3 -D_WIN32_WINNT=0x0600

# Detecta o sistema operacional para definir regras específicas
ifeq ($(OS),Windows_NT)
	# Configurações para Windows
	TARGET = bin/simulador.exe
	LDFLAGS = 
	MKDIR_CMD = if not exist bin mkdir bin
	CLEAN_CMD = del /Q /S bin\* src\*.o 2>nul || exit 0
else
	# Configurações para Linux/Mac
	TARGET = bin/simulador
	LDFLAGS = -lpthread
	MKDIR_CMD = mkdir -p bin
	CLEAN_CMD = rm -rf bin/* src/*.o
endif

# Lista de arquivos C e Objetos
SRCS = src/main.c src/relogio_global.c src/semaforo_transito.c
OBJS = $(SRCS:.c=.o)

# Regra principal (a primeira a rodar se digitar apenas "make")
all: build_dir $(TARGET)

# Cria a pasta bin se não existir
build_dir:
	@$(MKDIR_CMD)

# Linka os arquivos objeto gerando o executável na pasta bin
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compila arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar a compilação (arquivos temporários gerados)
clean:
	@$(CLEAN_CMD)
	@echo "Limpeza concluida."
