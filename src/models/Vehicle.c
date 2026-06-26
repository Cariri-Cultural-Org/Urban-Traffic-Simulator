/*
 * What it does?
 * Moves vehicle threads through roads while respecting cells and signals.
 */
#include "CityMap.h"
#include "GlobalClock.h"
#include "Intersection.h"
#include "Vehicle.h"

static int vehicle_occupy_start(Vehicle *vehicle)
{
    CityMap *city = (CityMap *)vehicle->city_map;
    Cell *cell = road_get_cell(vehicle->current_road, vehicle->road_cell_index);

    if (!city || !cell)
        return 0;

    pthread_mutex_lock(&city->state_mutex);
    int occupied = cell_try_occupy(cell, vehicle);

    if (occupied)
    {
        vehicle->position.row = cell->row;
        vehicle->position.col = cell->col;
    }

    pthread_mutex_unlock(&city->state_mutex);
    return occupied;
}

static void vehicle_release_current_cell(Vehicle *vehicle)
{
    CityMap *city = (CityMap *)vehicle->city_map;
    Cell *cell = road_get_cell(vehicle->current_road, vehicle->road_cell_index);

    if (!city || !cell)
        return;

    pthread_mutex_lock(&city->state_mutex);
    cell_release(cell);
    pthread_mutex_unlock(&city->state_mutex);
}

static VehicleDirection vehicle_forward_direction_for_road(const Road *road)
{
    if (!road)
        return VEHICLE_DIRECTION_RIGHT;

    return (road->orientation == ROAD_HORIZONTAL)
               ? VEHICLE_DIRECTION_RIGHT
               : VEHICLE_DIRECTION_DOWN;
}

static void vehicle_set_direction_from_road(Vehicle *vehicle)
{
    if (!vehicle || !vehicle->current_road)
        return;

    if (vehicle->current_road->orientation == ROAD_HORIZONTAL)
    {
        vehicle->direction = (vehicle->current_road->type == ROAD_ONE_WAY)
                                 ? vehicle_forward_direction_for_road(vehicle->current_road)
                                 : ((vehicle->current_road->id % 2 == 0)
                                        ? VEHICLE_DIRECTION_RIGHT
                                        : VEHICLE_DIRECTION_LEFT);
    }
    else
    {
        vehicle->direction = (vehicle->current_road->type == ROAD_ONE_WAY)
                                 ? vehicle_forward_direction_for_road(vehicle->current_road)
                                 : ((vehicle->current_road->id % 2 == 0)
                                        ? VEHICLE_DIRECTION_DOWN
                                        : VEHICLE_DIRECTION_UP);
    }
}

static void vehicle_update_position(Vehicle *vehicle, Cell *cell)
{
    if (!vehicle || !cell)
        return;

    vehicle->position.row = cell->row;
    vehicle->position.col = cell->col;
}

static int vehicle_next_index(const Vehicle *vehicle, int current_index, int *next_index)
{
    if (!vehicle || !next_index)
        return 0;

    switch (vehicle->direction)
    {
    case VEHICLE_DIRECTION_RIGHT:
    case VEHICLE_DIRECTION_DOWN:
        *next_index = current_index + 1;
        break;
    case VEHICLE_DIRECTION_LEFT:
    case VEHICLE_DIRECTION_UP:
        *next_index = current_index - 1;
        break;
    default:
        return 0;
    }

    return 1;
}

static int vehicle_route_contains_road(const Vehicle *vehicle, Road *road)
{
    if (!vehicle || !road)
        return -1;

    for (int i = 0; i < vehicle->route_length; i++)
    {
        if (vehicle->route[i] == road)
            return i;
    }

    return -1;
}

static int vehicle_roads_meet_at_intersection(
    const Intersection *intersection,
    const Road *current_road,
    const Road *next_road
)
{
    if (!intersection || !current_road || !next_road || current_road == next_road)
        return 0;

    if (current_road->orientation == ROAD_HORIZONTAL)
    {
        return intersection->horizontal_road == current_road &&
               intersection->vertical_road == next_road;
    }

    return intersection->vertical_road == current_road &&
           intersection->horizontal_road == next_road;
}

static void vehicle_set_direction_after_turn(Vehicle *vehicle, const Road *next_road)
{
    if (!vehicle || !next_road)
        return;

    if (next_road->type == ROAD_ONE_WAY)
    {
        vehicle->direction = vehicle_forward_direction_for_road(next_road);
        return;
    }

    if (next_road->orientation == ROAD_HORIZONTAL)
    {
        vehicle->direction = (vehicle->direction == VEHICLE_DIRECTION_UP)
                                 ? VEHICLE_DIRECTION_LEFT
                                 : VEHICLE_DIRECTION_RIGHT;
    }
    else
    {
        vehicle->direction = (vehicle->direction == VEHICLE_DIRECTION_LEFT)
                                 ? VEHICLE_DIRECTION_UP
                                 : VEHICLE_DIRECTION_DOWN;
    }
}

static void vehicle_follow_route_if_possible(Vehicle *vehicle, Intersection *intersection)
{
    if (!vehicle || !intersection || vehicle->route_length <= 1)
        return;

    int next_route_index = (vehicle->route_index + 1) % vehicle->route_length;
    Road *next_road = vehicle->route[next_route_index];

    if (!vehicle_roads_meet_at_intersection(intersection, vehicle->current_road, next_road))
        return;

    int next_index = (next_road->orientation == ROAD_HORIZONTAL)
                         ? vehicle->position.col
                         : vehicle->position.row;

    if (next_index < 0 || next_index >= next_road->cell_count)
        return;

    vehicle->current_road = next_road;
    vehicle->road_cell_index = next_index;
    vehicle->route_index = next_route_index;
    vehicle_set_direction_after_turn(vehicle, next_road);
}

static int vehicle_step(Vehicle *vehicle, int tick)
{
    (void)tick;

    CityMap *city = (CityMap *)vehicle->city_map;
    Road *road = vehicle->current_road;

    if (!city || !road)
        return 0;

    int current_index = vehicle->road_cell_index;
    int next_index = -1;

    if (!vehicle_next_index(vehicle, current_index, &next_index))
        return 0;

    if (next_index < 0 || next_index >= road->cell_count)
        return 0;

    Cell *origin = road_get_cell(road, current_index);
    Cell *destination = road_get_cell(road, next_index);

    if (!origin || !destination)
        return 0;

    Intersection *intersection = city_map_get_intersection(
        city,
        destination->row,
        destination->col
    );
    int priority_requested = 0;

    if (intersection)
    {
        if (vehicle->is_ambulance)
        {
            intersection_request_priority(intersection, road->orientation);
            priority_requested = 1;
        }

        if (!intersection_wait_for_green(intersection, road->orientation))
        {
            if (priority_requested)
                intersection_release_priority(intersection);

            return 0;
        }
    }

    if (!cell_wait_until_free(destination))
        return 0;

    pthread_mutex_lock(&city->state_mutex);
    int moved = cell_move(origin, destination, vehicle);

    if (moved)
    {
        vehicle->road_cell_index = next_index;
        vehicle_update_position(vehicle, destination);
    }

    pthread_mutex_unlock(&city->state_mutex);

    if (priority_requested)
        intersection_release_priority(intersection);

    if (moved && intersection)
        vehicle_follow_route_if_possible(vehicle, intersection);

    return moved;
}

void vehicle_init(
    Vehicle *vehicle,
    int id,
    int speed,
    Road *road,
    int road_cell_index,
    void *city_map
)
{
    vehicle->id = id;
    vehicle->speed = speed;
    vehicle->current_road = road;
    vehicle->road_cell_index = road_cell_index;

    Cell *cell = road_get_cell(road, road_cell_index);
    vehicle->position.row = cell ? cell->row : 0;
    vehicle->position.col = cell ? cell->col : 0;
    vehicle->route[0] = road;
    vehicle->route_length = road ? 1 : 0;
    vehicle->route_index = 0;
    vehicle->active = 1;
    vehicle->is_ambulance = 0;
    vehicle->city_map = city_map;
    vehicle_set_direction_from_road(vehicle);
}

void vehicle_set_route(Vehicle *vehicle, Road **route, int route_length)
{
    if (!vehicle || !route || route_length <= 0)
        return;

    if (route_length > VEHICLE_MAX_ROUTE_LENGTH)
        route_length = VEHICLE_MAX_ROUTE_LENGTH;

    vehicle->route_length = 0;

    for (int i = 0; i < route_length; i++)
    {
        if (!route[i])
            continue;

        vehicle->route[vehicle->route_length++] = route[i];
    }

    if (vehicle->route_length == 0)
        return;

    int route_index = vehicle_route_contains_road(vehicle, vehicle->current_road);
    vehicle->route_index = (route_index >= 0) ? route_index : 0;
}

void vehicle_set_ambulance(Vehicle *vehicle, int is_ambulance)
{
    if (vehicle)
        vehicle->is_ambulance = is_ambulance;
}

void *vehicle_thread(void *arg)
{
    Vehicle *vehicle = (Vehicle *)arg;

    if (!vehicle)
        return NULL;

    int tick = clock_get_tick();

    while (simulation_is_running() && !vehicle_occupy_start(vehicle))
    {
        wait_next_tick(tick);
        tick = clock_get_tick();
    }

    int last_tick = clock_get_tick();

    while (simulation_is_running() && vehicle->active)
    {
        wait_next_tick(last_tick);
        int current_tick = clock_get_tick();

        if (current_tick == last_tick)
            continue;

        last_tick = current_tick;

        if (!simulation_is_running())
            break;

        if (vehicle->speed <= 1 || (current_tick % vehicle->speed) == 0)
            vehicle_step(vehicle, current_tick);
    }

    vehicle_release_current_cell(vehicle);
    vehicle->active = 0;

    return NULL;
}
