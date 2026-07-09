# Compilador e Flags
CC = gcc
CFLAGS = -Wall -Wextra -g3 -D_WIN32_WINNT=0x0600 -Isrc

# Detecta o sistema operacional para definir regras específicas
ifeq ($(OS),Windows_NT)
	# Configurações para Windows
	TARGET = bin/traffic-simulator.exe
	TEST_INTERSECTION_TARGET = bin/test_intersection_priority.exe
	TEST_VEHICLE_TARGET = bin/test_vehicle_invariants.exe
	TEST_RENDERER_TARGET = bin/test_city_map_renderer.exe
	TEST_INTERSECTION_RUN = $(TEST_INTERSECTION_TARGET)
	TEST_VEHICLE_RUN = $(TEST_VEHICLE_TARGET)
	TEST_RENDERER_RUN = $(TEST_RENDERER_TARGET)
	LDFLAGS =
	MKDIR_CMD = if not exist bin mkdir bin
	CLEAN_CMD = del /Q /S bin\* src\*.o src\models\*.o 2>nul || exit 0
else
	# Configurações para Linux/Mac
	TARGET = bin/traffic-simulator
	TEST_INTERSECTION_TARGET = bin/test_intersection_priority
	TEST_VEHICLE_TARGET = bin/test_vehicle_invariants
	TEST_RENDERER_TARGET = bin/test_city_map_renderer
	TEST_INTERSECTION_RUN = ./$(TEST_INTERSECTION_TARGET)
	TEST_VEHICLE_RUN = ./$(TEST_VEHICLE_TARGET)
	TEST_RENDERER_RUN = ./$(TEST_RENDERER_TARGET)
	LDFLAGS = -lpthread
	MKDIR_CMD = mkdir -p bin
	CLEAN_CMD = rm -rf bin/* src/*.o src/models/*.o
endif

# Lista de arquivos C e Objetos
SRCS = src/main.c src/models/SimulationOutput.c src/models/vehicle_thread.c src/models/GlobalClock.c src/models/TrafficLight.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/CityMapRenderer.c src/models/city_map_utils.c src/models/Vehicle.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard src/models/*.h)

TEST_INTERSECTION_SRCS = tests/test_intersection_priority.c src/models/GlobalClock.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/city_map_utils.c
TEST_VEHICLE_SRCS = tests/test_vehicle_invariants.c src/models/SimulationOutput.c src/models/CityMapRenderer.c src/models/GlobalClock.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/city_map_utils.c src/models/vehicle_thread.c
TEST_RENDERER_SRCS = tests/test_city_map_renderer.c src/models/CityMapRenderer.c src/models/GlobalClock.c src/models/Cell.c src/models/Road.c src/models/Intersection.c src/models/CityMap.c src/models/city_map_utils.c
TEST_TARGETS = $(TEST_INTERSECTION_TARGET) $(TEST_VEHICLE_TARGET) $(TEST_RENDERER_TARGET)

# Regra principal (a primeira a rodar se digitar apenas "make")
all: build_dir $(TARGET)

# Cria a pasta bin se não existir
build_dir:
	@$(MKDIR_CMD)

# Linka os arquivos objeto gerando o executável na pasta bin
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(TEST_INTERSECTION_TARGET): $(TEST_INTERSECTION_SRCS)
	$(CC) $(CFLAGS) -o $(TEST_INTERSECTION_TARGET) $(TEST_INTERSECTION_SRCS) $(LDFLAGS)

$(TEST_VEHICLE_TARGET): $(TEST_VEHICLE_SRCS)
	$(CC) $(CFLAGS) -o $(TEST_VEHICLE_TARGET) $(TEST_VEHICLE_SRCS) $(LDFLAGS)

$(TEST_RENDERER_TARGET): $(TEST_RENDERER_SRCS)
	$(CC) $(CFLAGS) -o $(TEST_RENDERER_TARGET) $(TEST_RENDERER_SRCS) $(LDFLAGS)

test: build_dir $(TEST_TARGETS)
	$(TEST_INTERSECTION_RUN)
	$(TEST_VEHICLE_RUN)
	$(TEST_RENDERER_RUN)

# Compila arquivos .c em .o
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar a compilação (arquivos temporários gerados)
clean:
	@$(CLEAN_CMD)
	@echo "Cleanup completed."

.PHONY: all build_dir clean test
