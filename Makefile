# Compilador e Flags
CC = gcc
CFLAGS = -Wall -Wextra -g3 -D_WIN32_WINNT=0x0600 -Isrc

# Detecta o sistema operacional para definir regras específicas
ifeq ($(OS),Windows_NT)
	# Configurações para Windows
	TARGET = bin/traffic-simulator.exe
	LDFLAGS =
	MKDIR_CMD = if not exist bin mkdir bin
	CLEAN_CMD = del /Q /S bin\* src\*.o src\models\*.o 2>nul || exit 0
else
	# Configurações para Linux/Mac
	TARGET = bin/traffic-simulator
	LDFLAGS = -lpthread
	MKDIR_CMD = mkdir -p bin
	CLEAN_CMD = rm -rf bin/* src/*.o src/models/*.o
endif

# Lista de arquivos C e Objetos
SRCS = src/main.c src/models/SimulationOutput.c src/models/vehicle_thread.c src/models/GlobalClock.c src/models/TrafficLight.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/CityMapRenderer.c src/models/city_map_utils.c src/models/Vehicle.c src/models/Ambulance.c
OBJS = $(SRCS:.c=.o)
TEST_TARGET = bin/test-increment-2-3
TEST_SRCS = tests/test_increment_2_3.c src/models/SimulationOutput.c src/models/vehicle_thread.c src/models/GlobalClock.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/CityMapRenderer.c src/models/city_map_utils.c

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

test: build_dir $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) $(LDFLAGS)

# Regra para limpar a compilação (arquivos temporários gerados)
clean:
	@$(CLEAN_CMD)
	@echo "Cleanup completed."
