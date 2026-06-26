CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g3
LDFLAGS = -pthread
TARGET = bin/traffic-simulator

SRCS = \
	src/main.c \
	src/models/GlobalClock.c \
	src/models/TrafficLight.c \
	src/models/Cell.c \
	src/models/Road.c \
	src/models/Intersection.c \
	src/models/CityMap.c \
	src/models/Vehicle.c \
	src/models/Ambulance.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: build_dir $(TARGET)

build_dir:
	@mkdir -p bin

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	@rm -rf bin/* src/*.o src/models/*.o
	@echo "Build files removed."
