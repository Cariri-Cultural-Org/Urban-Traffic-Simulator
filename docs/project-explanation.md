# Project Explanation

The Urban Traffic Simulator represents a small city as a grid of synchronized cells. Cars and an ambulance move at the same time, wait for traffic signals, and avoid occupying the same cell.

The project is built to demonstrate operating system synchronization in C with `pthread`.

## Runtime Flow

The current MVP runs this sequence:

```text
create CityMap
initialize vehicles
start GlobalClock thread
start vehicle threads
repeat for 80 ticks:
    update intersection signals
    render the terminal snapshot
stop simulation
wake waiting threads
join threads
destroy shared resources
```

## City Map

`CityMap` owns:

- a 20 by 40 matrix of `Cell`;
- seven `Road` instances;
- 12 `Intersection` instances;
- a `state_mutex` used by the renderer and movement code.

The fixed road layout is:

```text
horizontal rows: 4, 10, 16
vertical cols:   8, 16, 24, 32
```

## Cells

Each `Cell` stores:

- row and column;
- occupied/free state;
- a pointer to the occupying `Vehicle`;
- a mutex.

`cell_move()` locks origin and destination cells in deterministic coordinate order. That prevents two vehicles from entering the same cell and reduces deadlock risk.

## Roads

A `Road` is an ordered list of pointers to existing cells in the city map. Roads do not own cells. This means a horizontal road and a vertical road share the same `Cell` at an intersection coordinate.

The MVP models one ordered lane per road. Two-way road metadata exists, but opposite lanes are not modeled as separate cell sequences yet.

## Intersections

An `Intersection` connects one horizontal road and one vertical road. It stores the currently green orientation:

```text
ROAD_HORIZONTAL -> horizontal vehicles may enter
ROAD_VERTICAL   -> vertical vehicles may enter
```

Vehicle threads wait on condition variables when the signal is red. The main thread toggles all intersection signals every five ticks.

## Vehicles

Each vehicle runs in its own thread. A vehicle waits for the global clock, checks whether it may enter the next cell, and then tries to move.

Speed is represented as ticks per movement:

- `1`: move every tick;
- `2`: move every second tick;
- `3`: move every third tick;
- `4`: move every fourth tick.

The ambulance is a normal `Vehicle` with `is_ambulance = 1`. When it approaches an intersection, it requests priority and sets the signal green for its direction.

## Rendering

`main.c` renders a terminal snapshot on every tick.

```text
C = car
A = ambulance
H = horizontal green
V = vertical green
- = horizontal road
| = vertical road
```

## Current Limitations

- Roads are fixed at compile time.
- Vehicles do not use configurable routes.
- Two-way roads are not represented with separate lanes.
- Rendering is terminal-only.
- There is no formal automated test suite yet.

## Related Documents

- [Operating System Concepts](os-concepts.md)
- [Implementation Plan](implementation-plan.md)
- [Virtual Memory Notes](virtual-memory.md)
