# Operating System Concepts Used By The Simulator

This project is intentionally small, but it exercises several core operating system ideas: threads, shared memory, mutexes, condition variables, race conditions, and deadlock prevention.

## Processes And Threads

A process owns memory and resources. A thread is an execution flow inside a process. In this simulator, all vehicles run inside the same process, but each vehicle has its own `pthread_t`.

Because the threads share memory, they can all read the same `CityMap`. That is useful, but it also means shared state must be protected.

## Shared Memory

The city grid, roads, intersections, and clock are shared by the main thread and vehicle threads.

Important shared objects:

- `CityMap`: owns cells, roads, and intersections;
- `Cell`: stores cell occupancy and the occupying vehicle;
- `Intersection`: stores the current green direction;
- `global_tick`: stores the current simulation time.

## Race Conditions

A race condition happens when two threads access the same data at the same time and the final result depends on timing.

Example in this project:

```text
Vehicle A sees a destination cell as free.
Vehicle B sees the same destination cell as free.
Both try to enter the cell.
```

`cell_try_occupy()` and `cell_move()` prevent that by locking the involved cells before changing occupancy.

## Mutexes

A mutex allows only one thread to enter a critical section at a time.

The simulator uses mutexes in several places:

- each `Cell` has a mutex for occupancy;
- each `Intersection` has a mutex for signal state;
- `GlobalClock` has a mutex for `global_tick`;
- `CityMap` has `state_mutex` for consistent rendered snapshots.

## Condition Variables

Condition variables let threads sleep until an event occurs. They avoid busy waiting.

The simulator uses condition variables for:

- clock ticks: `clock_cond`;
- horizontal intersection traffic: `horizontal_cond`;
- vertical intersection traffic: `vertical_cond`.

Vehicle threads wait on these condition variables instead of repeatedly checking values in a CPU-burning loop.

## Deadlock Prevention

Deadlock can happen when threads wait forever for locks held by each other.

The simulator follows these rules:

- a vehicle does not wait for the next tick while holding a cell mutex;
- a vehicle does not wait for a green signal while holding the city snapshot mutex;
- cell movement locks cells in deterministic coordinate order;
- shutdown broadcasts both the clock condition and all intersection conditions.

## Related Documents

- [Project Explanation](project-explanation.md)
- [Implementation Plan](implementation-plan.md)
- [Virtual Memory Notes](virtual-memory.md)
