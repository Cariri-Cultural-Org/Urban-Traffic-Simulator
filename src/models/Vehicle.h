/*
 * What it does?
 * Declares vehicles, their thread state, speed, and current position.
 */
#ifndef VEHICLE_H
#define VEHICLE_H

#include <pthread.h>

#include "Road.h"

#define VEHICLE_MAX_ROUTE_LENGTH 8

typedef enum
{
    VEHICLE_DIRECTION_RIGHT,
    VEHICLE_DIRECTION_LEFT,
    VEHICLE_DIRECTION_DOWN,
    VEHICLE_DIRECTION_UP
} VehicleDirection;

typedef enum
{
    VEHICLE_SPEED_FAST = 1,
    VEHICLE_SPEED_MEDIUM = 2,
    VEHICLE_SPEED_SLOW = 4
} VehicleSpeed;

typedef struct
{
    int row;
    int col;
} VehiclePosition;

typedef struct Vehicle
{
    int id;
    int speed;
    Road *current_road;
    int road_cell_index;
    VehiclePosition position;
    VehicleDirection direction;
    Road *route[VEHICLE_MAX_ROUTE_LENGTH];
    int route_length;
    int route_index;

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
void vehicle_set_route(Vehicle *vehicle, Road **route, int route_length);
void vehicle_set_ambulance(Vehicle *vehicle, int is_ambulance);
void *vehicle_thread(void *arg);

#endif
