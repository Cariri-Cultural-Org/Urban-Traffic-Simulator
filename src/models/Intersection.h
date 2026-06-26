
#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "pthread_compat.h"
#include "Road.h"
#include "TrafficLight.h"

typedef struct Intersection
{
    int id;
    int row;
    int column;
    Road *horizontal_road;
    Road *vertical_road;

    RoadDirection green_direction;
    pthread_mutex_t mutex; // Bloqueia passagem durante o sinal verde
    pthread_cond_t horizontal_cond;
    pthread_cond_t vertical_cond;

    // Suporte à ambulância
    int ambulance_present;
    RoadDirection ambulance_direction;
} Intersection;

Intersection *intersection_create(int id, int row, int column,
                                  Road *horizontal_road, Road *vertical_road);
void intersection_destroy(Intersection *intersection);
void intersection_toggle_signal(Intersection *intersection);
void intersection_wait_green(Intersection *intersection, RoadDirection road_direction);

#endif
