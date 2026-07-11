
#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "pthread_compat.h"
#include "Road.h"

typedef struct Intersection
{
    int id;
    int row;
    int column;
    Road *horizontal_road;
    Road *vertical_road;

    RoadDirection green_direction;
    RoadDirection previous_green_direction;
    pthread_mutex_t mutex; // Bloqueia passagem durante o sinal verde
    pthread_cond_t horizontal_cond;
    pthread_cond_t vertical_cond;

    // Suporte à ambulância
    int ambulance_present;
    RoadDirection ambulance_direction;

    int crossing_occupied;
    RoadDirection crossing_direction;
    pthread_cond_t crossing_clear_cond;
} Intersection;

Intersection *intersection_create(int id, int row, int column,
                                  Road *horizontal_road, Road *vertical_road);
void intersection_destroy(Intersection *intersection);
void intersection_toggle_signal(Intersection *intersection);
void intersection_wait_green(Intersection *intersection, RoadDirection road_direction);
int intersection_request_ambulance_priority(Intersection *intersection, RoadDirection road_direction);
void intersection_clear_ambulance_priority(Intersection *intersection);
void intersection_finish_crossing(Intersection *intersection, int clear_ambulance_priority);

#endif
