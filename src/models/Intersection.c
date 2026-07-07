#include <stdlib.h>
#include "Intersection.h"
#include "Road.h"

Intersection *intersection_create(int id, int row, int column, Road *horizontal_road, Road *vertical_road)
{
    Intersection *intersection = malloc(sizeof(Intersection));
    if (!intersection)
        return NULL;

    intersection->id = id;
    intersection->row = row;
    intersection->column = column;
    intersection->horizontal_road = horizontal_road;
    intersection->vertical_road = vertical_road;
    intersection->green_direction = ROAD_HORIZONTAL;

    intersection->ambulance_present = 0; // Ambulância inativa por padrão
    intersection->ambulance_direction = ROAD_HORIZONTAL;

    pthread_mutex_init(&intersection->mutex, NULL);
    pthread_cond_init(&intersection->horizontal_cond, NULL);
    pthread_cond_init(&intersection->vertical_cond, NULL);

    return intersection;
}

void intersection_destroy(Intersection *intersection)
{
    if (!intersection)
        return;

    pthread_cond_destroy(&intersection->vertical_cond);
    pthread_cond_destroy(&intersection->horizontal_cond);
    pthread_mutex_destroy(&intersection->mutex);
    free(intersection);
}

void intersection_toggle_signal(Intersection *intersection)
{
    pthread_mutex_lock(&intersection->mutex);

    /*
     * Se a ambulância está presente, verifica se a sua via já está verde.
     * Em caso positivo, não alterna — ela tem prioridade.
     */
    if (intersection->ambulance_present)
    {
        if (intersection->ambulance_direction == ROAD_HORIZONTAL &&
            intersection->green_direction == ROAD_HORIZONTAL)
        {
            pthread_mutex_unlock(&intersection->mutex);
            return;
        }
        if (intersection->ambulance_direction == ROAD_VERTICAL &&
            intersection->green_direction == ROAD_VERTICAL)
        {
            pthread_mutex_unlock(&intersection->mutex);
            return;
        }
    }

    if (intersection->green_direction == ROAD_HORIZONTAL)
    {
        intersection->green_direction = ROAD_VERTICAL;
        pthread_cond_broadcast(&intersection->vertical_cond); /* acorda carros da via vertical */
    }
    else
    {
        intersection->green_direction = ROAD_HORIZONTAL;
        pthread_cond_broadcast(&intersection->horizontal_cond); /* acorda carros da via horizontal */
    }

    pthread_mutex_unlock(&intersection->mutex);
}


void intersection_wait_green(Intersection *intersection, RoadDirection road_direction)
{
    if (road_direction == ROAD_HORIZONTAL)
    {
        while (simulation_running && intersection->green_direction != ROAD_HORIZONTAL)
            pthread_cond_wait(&intersection->horizontal_cond, &intersection->mutex);
    }
    else
    {
        while (simulation_running && intersection->green_direction != ROAD_VERTICAL)
            pthread_cond_wait(&intersection->vertical_cond, &intersection->mutex);
    }
}
