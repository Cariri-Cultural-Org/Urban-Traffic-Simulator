#include "vehicle_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "models/GlobalClock.h"
#include "models/Intersection.h"
#include "models/Vehicle.h"

int cell_is_free(Position position)
{
    (void)position;
    return 1;
}

void cell_occupy(Position position, int vehicle_id)
{
    (void)position;
    (void)vehicle_id;
}

void cell_release_position(Position position)
{
    (void)position;
}

void wait_tick(void)
{
    int current_tick = global_tick;
    wait_next_tick(current_tick);
}

int signal_is_green(Position intersection, Direction direction)
{
    (void)intersection;
    (void)direction;
    return 1;
}

void wait_green_signal(Position intersection, Direction direction)
{
    while (!signal_is_green(intersection, direction))
        wait_tick();
}

static const char *direction_to_string(Direction direction)
{
    switch (direction)
    {
    case DIRECTION_NORTH:
        return "NORTH";
    case DIRECTION_SOUTH:
        return "SOUTH";
    case DIRECTION_EAST:
        return "EAST";
    case DIRECTION_WEST:
        return "WEST";
    default:
        return "?";
    }
}

static const char *vehicle_type_to_string(VehicleType type)
{
    return type == VEHICLE_TYPE_AMBULANCE ? "AMBULANCE" : "CAR";
}

static int positions_are_equal(Position a, Position b)
{
    return a.row == b.row && a.column == b.column;
}

static int positions_are_adjacent(Position origin, Position destination)
{
    int row_delta = destination.row - origin.row;
    int column_delta = destination.column - origin.column;

    return abs(row_delta) + abs(column_delta) == 1;
}

static Direction direction_between(Position origin, Position destination)
{
    if (destination.row < origin.row)
        return DIRECTION_NORTH;
    if (destination.row > origin.row)
        return DIRECTION_SOUTH;
    if (destination.column > origin.column)
        return DIRECTION_EAST;

    return DIRECTION_WEST;
}

static int direction_matches_road(Direction direction, const Road *road)
{
    if (!road)
        return 0;

    if (road->direction == ROAD_HORIZONTAL)
        return direction == DIRECTION_EAST || direction == DIRECTION_WEST;

    return direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH;
}

static int road_index_for_position(const Road *road, Position position)
{
    if (!road)
        return -1;

    for (int i = 0; i < road->cell_count; i++)
    {
        Cell *cell = road_get_cell(road, i);

        if (cell && cell->row == position.row && cell->column == position.column)
            return i;
    }

    return -1;
}

static Road *find_movement_road(
    CityMap *city_map,
    Position origin,
    Position destination,
    int *origin_index,
    int *destination_index
)
{
    if (!city_map)
        return NULL;

    for (int i = 0; i < city_map->road_count; i++)
    {
        Road *road = city_map->roads[i];
        int from = road_index_for_position(road, origin);
        int to = road_index_for_position(road, destination);

        if (from >= 0 && to >= 0 && abs(to - from) == 1)
        {
            *origin_index = from;
            *destination_index = to;
            return road;
        }
    }

    return NULL;
}

static int movement_respects_one_way(const Road *road, int origin_index, int destination_index)
{
    if (!road)
        return 0;

    if (road->type == ROAD_TWO_WAY)
        return 1;

    return destination_index > origin_index;
}

static int wait_signal_if_needed(ThreadVehicle *vehicle, Road *road, int destination_index)
{
    Intersection *intersection = road_get_intersection(road, destination_index);

    if (!intersection)
        return 1;

    if (!vehicle->city_map)
    {
        wait_green_signal(vehicle->route[vehicle->route_index], vehicle->direction);
        return 1;
    }

    pthread_mutex_lock(&intersection->mutex);
    intersection_wait_green(intersection, road->direction);
    pthread_mutex_unlock(&intersection->mutex);
    return 1;
}

static int occupy_real_cell(ThreadVehicle *vehicle, Cell *cell)
{
    return cell_try_occupy(cell, (struct Vehicle *)vehicle);
}

static int try_move_on_city_map(ThreadVehicle *vehicle, Position destination)
{
    int origin_index = -1;
    int destination_index = -1;
    Direction movement_direction;
    Road *road;
    Cell *origin_cell;
    Cell *destination_cell;

    if (!city_map_is_valid_position(vehicle->city_map, destination.row, destination.column))
        return 0;

    if (!positions_are_adjacent(vehicle->position, destination))
        return 0;

    movement_direction = direction_between(vehicle->position, destination);

    if (movement_direction != vehicle->direction)
        return 0;

    road = find_movement_road(
        vehicle->city_map,
        vehicle->position,
        destination,
        &origin_index,
        &destination_index
    );

    if (!road || !direction_matches_road(movement_direction, road))
        return 0;

    if (!movement_respects_one_way(road, origin_index, destination_index))
        return 0;

    if (!wait_signal_if_needed(vehicle, road, destination_index))
        return 0;

    origin_cell = road_get_cell(road, origin_index);
    destination_cell = road_get_cell(road, destination_index);

    if (!origin_cell || !destination_cell)
        return 0;

    if (!occupy_real_cell(vehicle, destination_cell))
    {
        vehicle->state = VEHICLE_STATE_WAITING_CELL;
        return 0;
    }

    cell_release(origin_cell);
    vehicle->current_road = road;
    vehicle->road_cell_index = destination_index;
    vehicle->position = destination;
    vehicle->route_index++;
    vehicle->state = VEHICLE_STATE_MOVING;
    return 1;
}

static int try_move_without_city_map(ThreadVehicle *vehicle, Position destination)
{
    if (!positions_are_adjacent(vehicle->position, destination))
        return 0;

    if (direction_between(vehicle->position, destination) != vehicle->direction)
        return 0;

    if (!signal_is_green(destination, vehicle->direction))
    {
        vehicle->state = VEHICLE_STATE_WAITING_SIGNAL;
        wait_green_signal(destination, vehicle->direction);
    }

    if (!cell_is_free(destination))
    {
        vehicle->state = VEHICLE_STATE_WAITING_CELL;
        return 0;
    }

    cell_release_position(vehicle->position);
    cell_occupy(destination, vehicle->id);
    vehicle->position = destination;
    vehicle->route_index++;
    vehicle->state = VEHICLE_STATE_MOVING;
    return 1;
}

static int try_move(ThreadVehicle *vehicle)
{
    Position destination;

    while (vehicle->route_index < vehicle->route_size &&
           positions_are_equal(vehicle->position, vehicle->route[vehicle->route_index]))
    {
        vehicle->route_index++;
    }

    if (vehicle->route_index >= vehicle->route_size)
        return 0;

    destination = vehicle->route[vehicle->route_index];

    if (vehicle->city_map)
        return try_move_on_city_map(vehicle, destination);

    return try_move_without_city_map(vehicle, destination);
}

static void occupy_initial_cell(ThreadVehicle *vehicle)
{
    Cell *cell;

    if (!vehicle->city_map)
    {
        cell_occupy(vehicle->position, vehicle->id);
        return;
    }

    cell = city_map_get_cell(vehicle->city_map, vehicle->position.row, vehicle->position.column);

    if (cell)
        occupy_real_cell(vehicle, cell);
}

static void release_current_cell(ThreadVehicle *vehicle)
{
    Cell *cell;

    if (!vehicle->city_map)
    {
        cell_release_position(vehicle->position);
        return;
    }

    cell = city_map_get_cell(vehicle->city_map, vehicle->position.row, vehicle->position.column);

    if (cell)
        cell_release(cell);
}

static void *vehicle_thread_run(void *arg)
{
    ThreadVehicle *vehicle = (ThreadVehicle *)arg;

    printf(
        "[%s #%d] started at (%d,%d) speed=%d\n",
        vehicle_type_to_string(vehicle->type),
        vehicle->id,
        vehicle->position.row,
        vehicle->position.column,
        (int)vehicle->speed
    );

    occupy_initial_cell(vehicle);

    while (simulation_running && vehicle->route_index < vehicle->route_size)
    {
        wait_tick();

        vehicle->tick_counter++;

        if (vehicle->tick_counter < (int)vehicle->speed)
            continue;

        vehicle->tick_counter = 0;

        if (try_move(vehicle))
        {
            printf(
                "[%s #%d] moved to (%d,%d) [%s]\n",
                vehicle_type_to_string(vehicle->type),
                vehicle->id,
                vehicle->position.row,
                vehicle->position.column,
                direction_to_string(vehicle->direction)
            );
        }
    }

    release_current_cell(vehicle);
    vehicle->state = VEHICLE_STATE_FINISHED;

    printf("[%s #%d] route completed. Finishing thread.\n",
           vehicle_type_to_string(vehicle->type), vehicle->id);

    return NULL;
}

void thread_vehicle_init(
    ThreadVehicle *vehicle,
    int id,
    VehicleType type,
    Position initial_position,
    Direction direction,
    Speed speed,
    Position *route,
    int route_size
)
{
    memset(vehicle, 0, sizeof(ThreadVehicle));

    vehicle->id = id;
    vehicle->type = type;
    vehicle->position = initial_position;
    vehicle->direction = direction;
    vehicle->speed = speed;
    vehicle->state = VEHICLE_STATE_MOVING;
    vehicle->route_size = route_size > MAX_ROUTE ? MAX_ROUTE : route_size;

    if (route && vehicle->route_size > 0)
        memcpy(vehicle->route, route, vehicle->route_size * sizeof(Position));
}

void thread_vehicle_attach_city_map(ThreadVehicle *vehicle, CityMap *city_map)
{
    if (!vehicle)
        return;

    vehicle->city_map = city_map;
}

int thread_vehicle_start(ThreadVehicle *vehicle)
{
    int result = pthread_create(&vehicle->thread, NULL, vehicle_thread_run, vehicle);

    if (result != 0)
    {
        fprintf(stderr, "[ERROR] pthread_create failed for vehicle #%d (code %d)\n",
                vehicle->id, result);
        return -1;
    }

    return 0;
}

void thread_vehicle_join(ThreadVehicle *vehicle)
{
    pthread_join(vehicle->thread, NULL);
}

int thread_vehicles_create_all(ThreadVehicle vehicles[], int quantity)
{
    if (quantity < MIN_VEHICLES || quantity > MAX_VEHICLES)
    {
        fprintf(stderr, "[ERROR] invalid quantity: %d (expected %d-%d)\n",
                quantity, MIN_VEHICLES, MAX_VEHICLES);
        return -1;
    }

    int failures = 0;

    for (int i = 0; i < quantity; i++)
    {
        if (thread_vehicle_start(&vehicles[i]) != 0)
            failures++;
    }

    return failures == 0 ? 0 : -1;
}

void thread_vehicles_join_all(ThreadVehicle vehicles[], int quantity)
{
    for (int i = 0; i < quantity; i++)
        thread_vehicle_join(&vehicles[i]);
}
