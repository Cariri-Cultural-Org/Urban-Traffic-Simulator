#ifndef VEHICLE_H
#define VEHICLE_H

#include "pthread_compat.h"

#include "Road.h"

#define VEHICLE_MAX_ROUTE_LENGTH 16

typedef enum
{
    VEHICLE_DIRECTION_RIGHT,
    VEHICLE_DIRECTION_LEFT,
    VEHICLE_DIRECTION_DOWN,
    VEHICLE_DIRECTION_UP
} VehicleDirection;

typedef struct Vehicle
{
    int id;
    int velocity;
    Road *current_road;
    int road_cell_index;
    int row;
    int column;
    VehicleDirection direction;
    Road *route[VEHICLE_MAX_ROUTE_LENGTH];
    int route_length;
    int route_index;

    pthread_t thread;
    int active;
    void *city_map;
} Vehicle;

void vehicle_init(
    Vehicle *vehicle,
    int id,
    int velocity,
    Road *road,
    int road_cell_index,
    void *city_map
);
int vehicle_place_on_road(Vehicle *vehicle, Road *road, int road_cell_index);
void vehicle_set_route(Vehicle *vehicle, Road **route, int route_length);

#endif /* VEHICLE_H */
