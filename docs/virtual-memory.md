# Virtual Memory Notes

Virtual memory is not implemented directly by this project, but it is relevant background for understanding how the simulator runs inside an operating system.

## What Virtual Memory Provides

Virtual memory gives each process the illusion of a private, continuous address space. The operating system and hardware memory management unit translate virtual addresses into physical memory locations.

This simulator benefits from virtual memory in the usual way every process does:

- the executable receives its own address space;
- heap allocations for `CityMap`, `Cell`, `Road`, and `Intersection` are isolated from other processes;
- thread stacks live in the same process address space but at distinct memory regions;
- invalid memory access can be trapped by the operating system.

## Pages And Page Tables

Memory is usually divided into fixed-size pages. A page table maps virtual pages to physical frames.

When the simulator allocates the city grid, the C runtime requests memory from the operating system. The OS decides which physical frames back those virtual addresses.

## Page Faults

A page fault occurs when a process accesses a virtual page that is not currently mapped or not currently available.

Common causes:

- the page has not been loaded yet;
- the page was swapped out;
- the process accessed invalid memory.

For this project, page faults are not part of the traffic logic. They are an operating system mechanism below the C code.

## Relation To Threads

All threads in this simulator share the same virtual address space because they belong to the same process. That is why they can all access the same `CityMap`.

Shared address space is the reason mutexes and condition variables are required: memory is shared, but updates still need coordination.

## Related Documents

- [Operating System Concepts](os-concepts.md)
- [Project Explanation](project-explanation.md)
