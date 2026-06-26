#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <pthread.h>

#include "Road.h"

typedef struct Intersection
{
    int id;
    int row;
    int col;
    Road *horizontal_road;
    Road *vertical_road;

    RoadOrientation green_orientation;
    pthread_mutex_t mutex;
    pthread_cond_t horizontal_cond;
    pthread_cond_t vertical_cond;

    int ambulance_waiting;
    RoadOrientation ambulance_orientation;
} Intersection;

Intersection *intersection_create(
    int id,
    int row,
    int col,
    Road *horizontal_road,
    Road *vertical_road
);
void intersection_destroy(Intersection *intersection);
void intersection_toggle_signal(Intersection *intersection);
int intersection_wait_for_green(Intersection *intersection, RoadOrientation orientation);
void intersection_request_priority(Intersection *intersection, RoadOrientation orientation);
void intersection_release_priority(Intersection *intersection);
void intersection_wake_all(Intersection *intersection);

#endif
