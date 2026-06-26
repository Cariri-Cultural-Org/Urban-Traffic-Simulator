#ifndef VEHICLE_THREAD_H
#define VEHICLE_THREAD_H

#include "models/pthread_compat.h"

#include "models/CityMap.h"

#define MAX_VEHICLES 20
#define MIN_VEHICLES 10
#define MAX_ROUTE 256

typedef enum
{
    DIRECTION_NORTH,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST
} Direction;

typedef enum
{
    SPEED_FAST = 1,
    SPEED_MEDIUM = 2,
    SPEED_SLOW = 4
} Speed;

typedef enum
{
    VEHICLE_TYPE_CAR,
    VEHICLE_TYPE_AMBULANCE
} VehicleType;

typedef enum
{
    VEHICLE_STATE_MOVING,
    VEHICLE_STATE_WAITING_CELL,
    VEHICLE_STATE_WAITING_SIGNAL,
    VEHICLE_STATE_FINISHED
} VehicleState;

typedef struct
{
    int row;
    int column;
} Position;

typedef struct
{
    int id;
    VehicleType type;
    Position position;
    Direction direction;
    Speed speed;
    VehicleState state;

    Position route[MAX_ROUTE];
    int route_size;
    int route_index;

    int tick_counter;

    CityMap *city_map;
    Road *current_road;
    int road_cell_index;

    pthread_t thread;
} ThreadVehicle;

void thread_vehicle_init(
    ThreadVehicle *vehicle,
    int id,
    VehicleType type,
    Position initial_position,
    Direction direction,
    Speed speed,
    Position *route,
    int route_size
);
void thread_vehicle_attach_city_map(ThreadVehicle *vehicle, CityMap *city_map);

int thread_vehicle_start(ThreadVehicle *vehicle);
void thread_vehicle_join(ThreadVehicle *vehicle);
int thread_vehicles_create_all(ThreadVehicle vehicles[], int quantity);
void thread_vehicles_join_all(ThreadVehicle vehicles[], int quantity);

int cell_is_free(Position position);
void cell_occupy(Position position, int vehicle_id);
void cell_release_position(Position position);
void wait_tick(void);
int signal_is_green(Position intersection, Direction direction);
void wait_green_signal(Position intersection, Direction direction);

#endif /* VEHICLE_THREAD_H */
