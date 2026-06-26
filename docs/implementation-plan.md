# Implementation Plan

This document tracks the MVP scope after the English refactor.

## Delivered In The MVP Branch

- English source filenames, identifiers, comments, and primary documentation.
- `CityMap` initialization and destruction in the main flow.
- A 20 by 40 fixed city grid.
- Three horizontal roads and four vertical roads.
- 12 intersections with one green orientation at a time.
- One `pthread` per vehicle.
- 14 vehicles, including one ambulance.
- Tick-based movement coordinated by `GlobalClock`.
- Blocking waits for red lights through condition variables.
- Ambulance priority at intersections.
- ASCII terminal rendering on each tick.
- Clean shutdown with broadcasts and thread joins.

## Validation Checklist

Run:

```bash
make
./bin/traffic-simulator
```

Expected result:

- the project compiles without warnings;
- the terminal renders 80 ticks;
- the process ends with `Simulation finished cleanly.`;
- no worker thread remains blocked at shutdown.

## Remaining Work

- Add configurable simulation duration, vehicle count, and signal period.
- Model separate lanes for opposite directions.
- Replace deterministic turning with explicit routes.
- Add automated tests for movement, intersections, and shutdown.
- Add sanitizer or Valgrind validation to the normal workflow.
- Consider a graphical or curses-based renderer.

## Synchronization Contracts

- `Cell` mutex protects occupancy.
- `CityMap.state_mutex` protects consistent movement/render snapshots.
- `Intersection.mutex` protects signal state and ambulance priority.
- `GlobalClock` mutex and condition variable protect tick updates.
- Threads must not wait for clock ticks or traffic signals while holding cell locks.

## Related Documents

- [Project Explanation](project-explanation.md)
- [Operating System Concepts](os-concepts.md)
- [Virtual Memory Notes](virtual-memory.md)
