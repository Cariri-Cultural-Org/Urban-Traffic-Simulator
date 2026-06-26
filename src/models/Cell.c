/*
 * What it does?
 * Implements cell locking, occupancy, release, and safe cell-to-cell movement.
 */
#include "Cell.h"

void cell_init(Cell *cell, int row, int col)
{
    cell->row = row;
    cell->col = col;
    cell->occupied = 0;
    cell->vehicle = NULL;
    pthread_mutex_init(&cell->mutex, NULL);
    pthread_cond_init(&cell->cond, NULL);
}

void cell_destroy(Cell *cell)
{
    pthread_cond_destroy(&cell->cond);
    pthread_mutex_destroy(&cell->mutex);
}

int cell_try_occupy(Cell *cell, struct Vehicle *vehicle)
{
    int occupied = 0;

    pthread_mutex_lock(&cell->mutex);

    if (!cell->occupied)
    {
        cell->occupied = 1;
        cell->vehicle = vehicle;
        occupied = 1;
        pthread_cond_broadcast(&cell->cond);
    }

    pthread_mutex_unlock(&cell->mutex);
    return occupied;
}

int cell_wait_until_free(Cell *cell)
{
    if (!cell)
        return 0;

    pthread_mutex_lock(&cell->mutex);

    while (cell->occupied)
        pthread_cond_wait(&cell->cond, &cell->mutex);

    pthread_mutex_unlock(&cell->mutex);
    return 1;
}

int cell_move(Cell *origin, Cell *destination, struct Vehicle *vehicle)
{
    if (!destination || !vehicle)
        return 0;

    if (!origin)
        return cell_try_occupy(destination, vehicle);

    if (origin == destination)
        return 1;

    pthread_mutex_lock(&origin->mutex);
    pthread_mutex_lock(&destination->mutex);

    int moved = 0;

    if (origin->occupied && origin->vehicle == vehicle && !destination->occupied)
    {
        destination->occupied = 1;
        destination->vehicle = vehicle;
        origin->occupied = 0;
        origin->vehicle = NULL;
        pthread_cond_broadcast(&destination->cond);
        pthread_cond_broadcast(&origin->cond);
        moved = 1;
    }

    pthread_mutex_unlock(&destination->mutex);
    pthread_mutex_unlock(&origin->mutex);

    return moved;
}

void cell_release(Cell *cell)
{
    pthread_mutex_lock(&cell->mutex);
    cell->occupied = 0;
    cell->vehicle = NULL;
    pthread_cond_broadcast(&cell->cond);
    pthread_mutex_unlock(&cell->mutex);
}
