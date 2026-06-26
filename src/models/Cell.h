/*
 * What it does?
 * Declares a synchronized city cell and safe occupancy operations.
 */
#ifndef CELL_H
#define CELL_H

#include <pthread.h>

struct Vehicle;

typedef struct
{
    int row;
    int col;
    int occupied;
    struct Vehicle *vehicle;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Cell;

void cell_init(Cell *cell, int row, int col);
void cell_destroy(Cell *cell);
int cell_try_occupy(Cell *cell, struct Vehicle *vehicle);
int cell_move(Cell *origin, Cell *destination, struct Vehicle *vehicle);
void cell_release(Cell *cell);
int cell_wait_until_free(Cell *cell);

#endif
