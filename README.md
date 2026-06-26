# Urban Traffic Simulator

This project is a concurrent urban traffic simulator written in C. It models a small city as a fixed grid of cells, starts one thread per vehicle, and coordinates movement with a global simulation clock.

The MVP focuses on operating systems concepts: threads, mutexes, condition variables, shared memory, and clean shutdown. Vehicles move through roads and intersections without sharing the same cell, traffic signals block conflicting flows, and an ambulance can request priority at intersections.

## How The Project Works

1. The city is stored as a 20 by 40 grid.
2. Roads reference cells that already exist in the city grid.
3. Intersections connect horizontal and vertical roads.
4. A global clock wakes the simulation every tick.
5. Each vehicle runs in its own thread.
6. The terminal renderer prints a snapshot of the city on every tick.

## City Grid

The central structure is `CityMap`, declared in `src/models/CityMap.h`.

```c
typedef struct
{
    int rows;
    int cols;
    Cell **cells;
    Road **roads;
    int road_count;
    Intersection **intersections;
    int intersection_count;
    pthread_mutex_t state_mutex;
} CityMap;
```

The map uses these dimensions:

```c
#define CITY_MAP_ROWS 20
#define CITY_MAP_COLS 40
```

Each position is a `Cell`. A cell knows its coordinates, whether it is occupied, which vehicle is there, and the mutex that protects that state.

## Roads And Intersections

The MVP creates three horizontal roads and four vertical roads:

```text
horizontal rows: 4, 10, 16
vertical cols:   8, 16, 24, 32
```

That produces 12 intersections. Each intersection has one active green direction at a time:

```c
typedef struct Intersection
{
    int row;
    int col;
    Road *horizontal_road;
    Road *vertical_road;
    RoadOrientation green_orientation;
    pthread_mutex_t mutex;
    pthread_cond_t horizontal_cond;
    pthread_cond_t vertical_cond;
} Intersection;
```

`Intersection` is the source of truth for traffic signals. The older standalone `TrafficLight` module remains available as a simple isolated traffic-light model, but the running MVP uses intersection signals.

## Global Clock

`GlobalClock` owns the shared tick counter. The clock thread sleeps for about 100 milliseconds, increments `global_tick`, and broadcasts `clock_cond` so worker threads can advance without busy waiting.

Simplified flow:

```text
clock thread sleeps
    -> increments global_tick
    -> broadcasts clock_cond
    -> vehicle threads wake and try one step
```

The executable runs for `SIMULATION_TICKS` ticks, currently set to 80 in `src/main.c`.

## Vehicle Threads

Each `Vehicle` stores its current road, cell index, position, speed, and thread handle. The speed value means "ticks per movement": speed 1 moves every tick, speed 2 moves every second tick, and so on.

Movement is synchronized in this order:

1. Wait for the next global tick.
2. If the destination is an intersection, wait for green.
3. Lock the city snapshot mutex.
4. Move between cells with deterministic cell locking.
5. Release the city snapshot mutex.

The ambulance uses the same worker path as normal cars, but it sets `is_ambulance = 1`. Before entering an intersection, it requests priority and forces the green direction to its current road orientation.

## Terminal Output

Run output is rendered directly in the terminal.

```text
C = normal car
A = ambulance
H = intersection with horizontal green
V = intersection with vertical green
- = horizontal road
| = vertical road
```

The display uses ANSI escape codes to redraw frames in place. If your terminal does not support ANSI escape sequences, the frames will still print, but they may appear as repeated blocks.

## Project Structure

```text
.
├── Makefile
├── README.md
├── docs/
│   ├── implementation-plan.md
│   ├── os-concepts.md
│   ├── project-explanation.md
│   └── virtual-memory.md
└── src/
    ├── main.c
    └── models/
        ├── Ambulance.c / Ambulance.h
        ├── AsciiElements.h
        ├── Cell.c / Cell.h
        ├── CityMap.c / CityMap.h
        ├── GlobalClock.c / GlobalClock.h
        ├── Intersection.c / Intersection.h
        ├── Road.c / Road.h
        ├── TrafficLight.c / TrafficLight.h
        └── Vehicle.c / Vehicle.h
```

## Requirements

You need:

- GCC or another C compiler compatible with `pthread`;
- GNU Make;
- a POSIX-like environment such as Linux, macOS, or WSL.

The current MVP is validated on Linux. For Windows, use WSL or a toolchain that provides pthread support.

## How To Run The Project

From the repository root:

```bash
make
./bin/traffic-simulator
```

You can also build and run in one command:

```bash
make run
```

To remove generated files:

```bash
make clean
```

## Validation

The minimum validation command is:

```bash
make
./bin/traffic-simulator
```

A successful run ends with:

```text
Simulation finished cleanly.
```

## Current MVP Limitations

The simulator is intentionally minimal:

- each road is represented as one ordered lane of cells;
- two-way roads are recorded as metadata, not as separate opposite lanes;
- vehicles move forward by increasing their road cell index and wrapping at the end;
- turning is deterministic and simple, not route-file driven;
- the renderer is a terminal view, not a graphical UI;
- the simulation has no command-line configuration yet.

## Contributor Notes

Keep source names, comments, and documentation in English. After changing shared behavior, update the README so it describes the implemented system, not planned behavior.

Before handing off changes, run:

```bash
make
./bin/traffic-simulator
```

When editing synchronization code, keep these rules:

- do not wait for a tick while holding a cell mutex;
- do not wait for a traffic signal while holding the city snapshot mutex;
- wake the clock and all intersection condition variables during shutdown;
- join every started thread before destroying shared memory.

## Logic Summary

- `CityMap` allocates the grid, roads, and intersections.
- `GlobalClock` creates a shared tick every 100 milliseconds.
- `Vehicle` threads wait for ticks and try to advance one cell at a time.
- `Intersection` condition variables block vehicles on red signals.
- The ambulance requests priority before entering an intersection.
- `main.c` toggles signals, renders each tick, stops the simulation, joins threads, and frees memory.

## Additional Documentation

- [Project Explanation](docs/project-explanation.md)
- [Implementation Plan](docs/implementation-plan.md)
- [Operating System Concepts](docs/os-concepts.md)
- [Virtual Memory Notes](docs/virtual-memory.md)
