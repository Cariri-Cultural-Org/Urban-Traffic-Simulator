#include <stdlib.h>

#include "GlobalClock.h"
#include "Intersection.h"

Intersection *intersection_create(
    int id,
    int row,
    int col,
    Road *horizontal_road,
    Road *vertical_road
)
{
    Intersection *intersection = malloc(sizeof(Intersection));

    if (!intersection)
        return NULL;

    intersection->id = id;
    intersection->row = row;
    intersection->col = col;
    intersection->horizontal_road = horizontal_road;
    intersection->vertical_road = vertical_road;
    intersection->green_orientation = ROAD_HORIZONTAL;
    intersection->ambulance_waiting = 0;
    intersection->ambulance_orientation = ROAD_HORIZONTAL;

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
}

void intersection_toggle_signal(Intersection *intersection)
{
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);

    if (intersection->ambulance_waiting)
    {
        intersection->green_orientation = intersection->ambulance_orientation;
    }
    else if (intersection->green_orientation == ROAD_HORIZONTAL)
    {
        intersection->green_orientation = ROAD_VERTICAL;
    }
    else
    {
        intersection->green_orientation = ROAD_HORIZONTAL;
    }

    if (intersection->green_orientation == ROAD_HORIZONTAL)
        pthread_cond_broadcast(&intersection->horizontal_cond);
    else
        pthread_cond_broadcast(&intersection->vertical_cond);

    pthread_mutex_unlock(&intersection->mutex);
}

int intersection_wait_for_green(Intersection *intersection, RoadOrientation orientation)
{
    if (!intersection)
        return 0;

    pthread_mutex_lock(&intersection->mutex);

    pthread_cond_t *cond = (orientation == ROAD_HORIZONTAL)
                               ? &intersection->horizontal_cond
                               : &intersection->vertical_cond;

    while (intersection->green_orientation != orientation && simulation_is_running())
        pthread_cond_wait(cond, &intersection->mutex);

    int can_enter = simulation_is_running() &&
                    intersection->green_orientation == orientation;

    pthread_mutex_unlock(&intersection->mutex);
    return can_enter;
}

void intersection_request_priority(Intersection *intersection, RoadOrientation orientation)
{
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);

    intersection->ambulance_waiting = 1;
    intersection->ambulance_orientation = orientation;
    intersection->green_orientation = orientation;

    if (orientation == ROAD_HORIZONTAL)
        pthread_cond_broadcast(&intersection->horizontal_cond);
    else
        pthread_cond_broadcast(&intersection->vertical_cond);

    pthread_mutex_unlock(&intersection->mutex);
}

void intersection_release_priority(Intersection *intersection)
{
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);
    intersection->ambulance_waiting = 0;
    pthread_cond_broadcast(&intersection->horizontal_cond);
    pthread_cond_broadcast(&intersection->vertical_cond);
    pthread_mutex_unlock(&intersection->mutex);
}

void intersection_wake_all(Intersection *intersection)
{
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);
    pthread_cond_broadcast(&intersection->horizontal_cond);
    pthread_cond_broadcast(&intersection->vertical_cond);
    pthread_mutex_unlock(&intersection->mutex);
}
