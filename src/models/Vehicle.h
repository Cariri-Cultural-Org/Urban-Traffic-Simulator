#ifndef VEHICLE_H
#define VEHICLE_H

#include <pthread.h>

#include "Road.h"

typedef struct Vehicle
{
    int id;
    int speed;
    Road *current_road;
    int road_cell_index;
    int row;
    int col;

    pthread_t thread;
    int active;
    int is_ambulance;
    void *city_map;
} Vehicle;

void vehicle_init(
    Vehicle *vehicle,
    int id,
    int speed,
    Road *road,
    int road_cell_index,
    void *city_map
);
void vehicle_set_ambulance(Vehicle *vehicle, int is_ambulance);
void *vehicle_thread(void *arg);

#endif
