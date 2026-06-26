#ifndef CELL_H
#define CELL_H

#include "pthread_compat.h"

// Forward declaration de Vehicle.
struct Vehicle;

typedef struct
{
    int row;
    int column;
    int occupied;
    struct Vehicle *vehicle;
    pthread_mutex_t mutex;
} Cell;

void cell_init(Cell *cell, int row, int column);
void cell_destroy(Cell *cell);
int cell_try_occupy(Cell *cell, struct Vehicle *vehicle);
void cell_release(Cell *cell);

#endif
