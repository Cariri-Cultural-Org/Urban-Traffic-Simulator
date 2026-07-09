#include <stdlib.h>
#include "GlobalClock.h"
#include "Intersection.h"
#include "Road.h"

static int intersection_direction_is_open_locked(
    const Intersection *intersection,
    RoadDirection road_direction
)
{
    if (intersection->ambulance_present)
        return road_direction == intersection->ambulance_direction;

    return road_direction == intersection->green_direction;
}

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
    intersection->previous_green_direction = ROAD_HORIZONTAL;

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

    if (intersection->ambulance_present)
    {
        if (intersection->green_direction != intersection->ambulance_direction)
        {
            intersection->green_direction = intersection->ambulance_direction;

            if (intersection->green_direction == ROAD_HORIZONTAL)
                pthread_cond_broadcast(&intersection->horizontal_cond);
            else
                pthread_cond_broadcast(&intersection->vertical_cond);
        }

        pthread_mutex_unlock(&intersection->mutex);
        return;
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
    const unsigned int wait_slice_ms = 100;

    if (road_direction == ROAD_HORIZONTAL)
    {
        while (simulation_running && !intersection_direction_is_open_locked(intersection, ROAD_HORIZONTAL))
            pthread_cond_timedwait_ms(&intersection->horizontal_cond, &intersection->mutex, wait_slice_ms);
    }
    else
    {
        while (simulation_running && !intersection_direction_is_open_locked(intersection, ROAD_VERTICAL))
            pthread_cond_timedwait_ms(&intersection->vertical_cond, &intersection->mutex, wait_slice_ms);
    }
}

int intersection_request_ambulance_priority(Intersection *intersection, RoadDirection road_direction)
{
    int changed_signal = 0;

    if (!intersection)
        return 0;

    pthread_mutex_lock(&intersection->mutex);

    if (!intersection->ambulance_present)
    {
        intersection->previous_green_direction = intersection->green_direction;
        intersection->ambulance_present = 1;
    }

    intersection->ambulance_direction = road_direction;

    if (intersection->green_direction != road_direction)
    {
        intersection->green_direction = road_direction;
        changed_signal = 1;

        if (road_direction == ROAD_HORIZONTAL)
            pthread_cond_broadcast(&intersection->horizontal_cond);
        else
            pthread_cond_broadcast(&intersection->vertical_cond);
    }

    pthread_mutex_unlock(&intersection->mutex);
    return changed_signal;
}

void intersection_clear_ambulance_priority(Intersection *intersection)
{
    RoadDirection restored_direction;

    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);

    if (!intersection->ambulance_present)
    {
        pthread_mutex_unlock(&intersection->mutex);
        return;
    }

    restored_direction = intersection->previous_green_direction;
    intersection->ambulance_present = 0;
    intersection->ambulance_direction = ROAD_HORIZONTAL;
    intersection->green_direction = restored_direction;

    if (restored_direction == ROAD_HORIZONTAL)
        pthread_cond_broadcast(&intersection->horizontal_cond);
    else
        pthread_cond_broadcast(&intersection->vertical_cond);

    pthread_mutex_unlock(&intersection->mutex);
}
