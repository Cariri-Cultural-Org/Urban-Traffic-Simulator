#include "vehicle_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SimulationOutput.h"
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
    int current_tick = global_clock_current_tick();
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

static const char *road_direction_to_string(RoadDirection direction)
{
    return direction == ROAD_HORIZONTAL ? "horizontal" : "vertical";
}

static char vehicle_type_to_symbol(VehicleType type)
{
    return type == VEHICLE_TYPE_AMBULANCE ? 'A' : 'C';
}

static struct Vehicle *vehicle_cell_pointer(ThreadVehicle *vehicle)
{
    return (struct Vehicle *)vehicle;
}

static int cell_precedes(const Cell *left, const Cell *right)
{
    if (left->row != right->row)
        return left->row < right->row;

    if (left->column != right->column)
        return left->column < right->column;

    return left < right;
}

static void lock_cell_pair(Cell *first, Cell *second)
{
    if (first == second)
    {
        pthread_mutex_lock(&first->mutex);
        return;
    }

    if (cell_precedes(second, first))
    {
        Cell *tmp = first;
        first = second;
        second = tmp;
    }

    pthread_mutex_lock(&first->mutex);
    pthread_mutex_lock(&second->mutex);
}

static void unlock_cell_pair(Cell *first, Cell *second)
{
    if (first == second)
    {
        pthread_mutex_unlock(&first->mutex);
        return;
    }

    if (cell_precedes(second, first))
    {
        Cell *tmp = first;
        first = second;
        second = tmp;
    }

    pthread_mutex_unlock(&second->mutex);
    pthread_mutex_unlock(&first->mutex);
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

static int lock_intersection_for_entry(
    ThreadVehicle *vehicle,
    Road *road,
    int destination_index,
    Intersection **locked_intersection
)
{
    Intersection *intersection = road_get_intersection(road, destination_index);

    *locked_intersection = NULL;

    if (!intersection)
        return 1;

    if (!vehicle->city_map)
    {
        wait_green_signal(vehicle->route[vehicle->route_index], vehicle->direction);
        return 1;
    }

    if (vehicle->type == VEHICLE_TYPE_AMBULANCE)
    {
        simulation_output_log(
            "[AMBULANCE #%d] priority requested at intersection #%d (%d,%d), direction=%s\n",
            vehicle->id,
            intersection->id,
            intersection->row,
            intersection->column,
            road_direction_to_string(road->direction)
        );

        intersection_request_ambulance_priority(intersection, road->direction);

        if (!simulation_running)
            return 0;

        simulation_output_log(
            "[AMBULANCE #%d] priority granted at intersection #%d (%d,%d), direction=%s\n",
            vehicle->id,
            intersection->id,
            intersection->row,
            intersection->column,
            road_direction_to_string(road->direction)
        );
    }

    pthread_mutex_lock(&intersection->mutex);
    if (intersection->green_direction != road->direction ||
        (intersection->ambulance_present &&
         intersection->ambulance_direction != road->direction))
    {
        vehicle->state = VEHICLE_STATE_WAITING_SIGNAL;
    }

    intersection_wait_green(intersection, road->direction);

    if (!simulation_running)
    {
        pthread_mutex_unlock(&intersection->mutex);
        return 0;
    }

    *locked_intersection = intersection;
    return 1;
}

static int occupy_real_cell(ThreadVehicle *vehicle, Cell *cell)
{
    int success = 0;

    if (!vehicle || !cell)
        return 0;

    city_map_lock_state(vehicle->city_map);
    pthread_mutex_lock(&cell->mutex);

    if (!cell->occupied)
    {
        cell->occupied = 1;
        cell->occupant_symbol = vehicle_type_to_symbol(vehicle->type);
        cell->vehicle = vehicle_cell_pointer(vehicle);
        success = 1;
    }

    pthread_mutex_unlock(&cell->mutex);
    city_map_unlock_state(vehicle->city_map);

    return success;
}

static int move_real_cell(ThreadVehicle *vehicle, Cell *origin_cell, Cell *destination_cell)
{
    int success = 0;

    if (!vehicle || !origin_cell || !destination_cell)
        return 0;

    city_map_lock_state(vehicle->city_map);
    lock_cell_pair(origin_cell, destination_cell);

    if (origin_cell->occupied &&
        origin_cell->vehicle == vehicle_cell_pointer(vehicle) &&
        !destination_cell->occupied)
    {
        origin_cell->occupied = 0;
        origin_cell->occupant_symbol = ' ';
        origin_cell->vehicle = NULL;

        destination_cell->occupied = 1;
        destination_cell->occupant_symbol = vehicle_type_to_symbol(vehicle->type);
        destination_cell->vehicle = vehicle_cell_pointer(vehicle);
        success = 1;
    }

    unlock_cell_pair(origin_cell, destination_cell);
    city_map_unlock_state(vehicle->city_map);

    return success;
}

static void release_real_cell(ThreadVehicle *vehicle, Cell *cell)
{
    if (!vehicle || !cell)
        return;

    city_map_lock_state(vehicle->city_map);
    pthread_mutex_lock(&cell->mutex);

    if (cell->vehicle == vehicle_cell_pointer(vehicle))
    {
        cell->occupied = 0;
        cell->occupant_symbol = ' ';
        cell->vehicle = NULL;
    }

    pthread_mutex_unlock(&cell->mutex);
    city_map_unlock_state(vehicle->city_map);
}

static int try_move_on_city_map(ThreadVehicle *vehicle, Position destination)
{
    int origin_index = -1;
    int destination_index = -1;
    Direction movement_direction;
    Road *road;
    Cell *origin_cell;
    Cell *destination_cell;
    Intersection *origin_intersection;
    Intersection *destination_intersection = NULL;

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

    if (!lock_intersection_for_entry(
            vehicle,
            road,
            destination_index,
            &destination_intersection))
    {
        return 0;
    }

    origin_cell = road_get_cell(road, origin_index);
    destination_cell = road_get_cell(road, destination_index);
    origin_intersection = road_get_intersection(road, origin_index);

    if (!origin_cell || !destination_cell)
    {
        if (destination_intersection)
            pthread_mutex_unlock(&destination_intersection->mutex);
        return 0;
    }

    if (!move_real_cell(vehicle, origin_cell, destination_cell))
    {
        if (destination_intersection)
            pthread_mutex_unlock(&destination_intersection->mutex);

        vehicle->state = VEHICLE_STATE_WAITING_CELL;
        simulation_output_log(
            "[%s #%d] waiting: cell (%d,%d) is occupied\n",
            vehicle_type_to_string(vehicle->type),
            vehicle->id,
            destination.row,
            destination.column
        );
        return 0;
    }

    if (destination_intersection)
    {
        destination_intersection->crossing_occupied = 1;
        destination_intersection->crossing_direction = road->direction;
        pthread_mutex_unlock(&destination_intersection->mutex);
    }

    if (origin_intersection)
    {
        intersection_finish_crossing(
            origin_intersection,
            vehicle->type == VEHICLE_TYPE_AMBULANCE
        );

        if (vehicle->type == VEHICLE_TYPE_AMBULANCE)
        {
            simulation_output_log(
                "[AMBULANCE #%d] priority released at intersection #%d (%d,%d)\n",
                vehicle->id,
                origin_intersection->id,
                origin_intersection->row,
                origin_intersection->column
            );
        }
    }

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
        simulation_output_log(
            "[%s #%d] waiting: cell (%d,%d) is occupied\n",
            vehicle_type_to_string(vehicle->type),
            vehicle->id,
            destination.row,
            destination.column
        );
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

static int occupy_initial_cell(ThreadVehicle *vehicle)
{
    Cell *cell;

    if (!vehicle->city_map)
    {
        cell_occupy(vehicle->position, vehicle->id);
        return 1;
    }

    cell = city_map_get_cell(vehicle->city_map, vehicle->position.row, vehicle->position.column);

    if (cell && occupy_real_cell(vehicle, cell))
        return 1;

    if (cell)
    {
        vehicle->state = VEHICLE_STATE_WAITING_CELL;
        simulation_output_log(
            "[%s #%d] could not occupy initial cell (%d,%d)\n",
            vehicle_type_to_string(vehicle->type),
            vehicle->id,
            vehicle->position.row,
            vehicle->position.column
        );
    }

    return 0;
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
    {
        Intersection *intersection = city_map_get_intersection(
            vehicle->city_map,
            vehicle->position.row,
            vehicle->position.column
        );

        release_real_cell(vehicle, cell);

        if (intersection)
            intersection_finish_crossing(
                intersection,
                vehicle->type == VEHICLE_TYPE_AMBULANCE
            );
    }
}

static void *vehicle_thread_run(void *arg)
{
    ThreadVehicle *vehicle = (ThreadVehicle *)arg;

    simulation_output_log(
        "[%s #%d] started at (%d,%d) speed=%d\n",
        vehicle_type_to_string(vehicle->type),
        vehicle->id,
        vehicle->position.row,
        vehicle->position.column,
        (int)vehicle->speed
    );

    if (!occupy_initial_cell(vehicle))
    {
        simulation_output_log("[%s #%d] did not start because initial cell is unavailable.\n",
                              vehicle_type_to_string(vehicle->type), vehicle->id);
        return NULL;
    }

    while (simulation_running && vehicle->route_index < vehicle->route_size)
    {
        wait_tick();

        if (!simulation_running)
            break;

        vehicle->tick_counter++;

        if (vehicle->tick_counter < (int)vehicle->speed)
            continue;

        vehicle->tick_counter = 0;

        if (try_move(vehicle))
        {
            simulation_output_log(
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

    if (vehicle->route_index >= vehicle->route_size)
    {
        simulation_output_log("[%s #%d] route completed. Finishing thread.\n",
                              vehicle_type_to_string(vehicle->type), vehicle->id);
    }
    else
    {
        simulation_output_log("[%s #%d] stopped by simulation shutdown.\n",
                              vehicle_type_to_string(vehicle->type), vehicle->id);
    }

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

int thread_vehicle_place(ThreadVehicle *vehicle)
{
    if (!vehicle)
        return 0;

    return occupy_initial_cell(vehicle);
}

int thread_vehicle_advance_one_step(ThreadVehicle *vehicle)
{
    if (!vehicle)
        return 0;

    return try_move(vehicle);
}

void thread_vehicle_release(ThreadVehicle *vehicle)
{
    if (!vehicle)
        return;

    release_current_cell(vehicle);
}

int thread_vehicle_start(ThreadVehicle *vehicle)
{
    int result = pthread_create(&vehicle->thread, NULL, vehicle_thread_run, vehicle);

    if (result != 0)
    {
        simulation_output_log("[ERROR] pthread_create failed for vehicle #%d (code %d)\n",
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
        simulation_output_log("[ERROR] invalid quantity: %d (expected %d-%d)\n",
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
