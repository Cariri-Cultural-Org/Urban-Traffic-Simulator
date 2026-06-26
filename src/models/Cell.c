#include "Cell.h"

void cell_init(Cell *cell, int row, int col)
{
    cell->row = row;
    cell->col = col;
    cell->occupied = 0;
    cell->vehicle = NULL;
    pthread_mutex_init(&cell->mutex, NULL);
}

void cell_destroy(Cell *cell)
{
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
    }

    pthread_mutex_unlock(&cell->mutex);
    return occupied;
}

static int cell_linear_index(const Cell *cell)
{
    return cell->row * 10000 + cell->col;
}

int cell_move(Cell *origin, Cell *destination, struct Vehicle *vehicle)
{
    if (!destination || !vehicle)
        return 0;

    if (!origin)
        return cell_try_occupy(destination, vehicle);

    if (origin == destination)
        return 1;

    Cell *first = origin;
    Cell *second = destination;

    if (cell_linear_index(destination) < cell_linear_index(origin))
    {
        first = destination;
        second = origin;
    }

    pthread_mutex_lock(&first->mutex);
    pthread_mutex_lock(&second->mutex);

    int moved = 0;

    if (origin->occupied && origin->vehicle == vehicle && !destination->occupied)
    {
        destination->occupied = 1;
        destination->vehicle = vehicle;
        origin->occupied = 0;
        origin->vehicle = NULL;
        moved = 1;
    }

    pthread_mutex_unlock(&second->mutex);
    pthread_mutex_unlock(&first->mutex);

    return moved;
}

void cell_release(Cell *cell)
{
    pthread_mutex_lock(&cell->mutex);
    cell->occupied = 0;
    cell->vehicle = NULL;
    pthread_mutex_unlock(&cell->mutex);
}
