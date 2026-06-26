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
        vehicle->row = cell->row;
        vehicle->col = cell->col;
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

static int vehicle_should_turn(const Vehicle *vehicle, int tick)
{
    if (vehicle->is_ambulance)
        return ((tick + vehicle->id) % 3) == 0;

    return ((tick + vehicle->id) % 5) == 0;
}

static void vehicle_turn_if_needed(Vehicle *vehicle, Intersection *intersection, int tick)
{
    if (!intersection || !vehicle_should_turn(vehicle, tick))
        return;

    Road *next_road = (vehicle->current_road->orientation == ROAD_HORIZONTAL)
                          ? intersection->vertical_road
                          : intersection->horizontal_road;

    if (!next_road)
        return;

    int next_index = (next_road->orientation == ROAD_HORIZONTAL)
                         ? vehicle->col
                         : vehicle->row;

    if (next_index < 0 || next_index >= next_road->cell_count)
        return;

    vehicle->current_road = next_road;
    vehicle->road_cell_index = next_index;
}

static int vehicle_step(Vehicle *vehicle, int tick)
{
    CityMap *city = (CityMap *)vehicle->city_map;
    Road *road = vehicle->current_road;

    if (!city || !road)
        return 0;

    int current_index = vehicle->road_cell_index;
    int next_index = current_index + 1;

    if (next_index >= road->cell_count)
        next_index = 0;

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

    pthread_mutex_lock(&city->state_mutex);
    int moved = cell_move(origin, destination, vehicle);

    if (moved)
    {
        vehicle->road_cell_index = next_index;
        vehicle->row = destination->row;
        vehicle->col = destination->col;
    }

    pthread_mutex_unlock(&city->state_mutex);

    if (priority_requested)
        intersection_release_priority(intersection);

    if (moved && intersection)
        vehicle_turn_if_needed(vehicle, intersection, tick);

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
    vehicle->row = cell ? cell->row : 0;
    vehicle->col = cell ? cell->col : 0;
    vehicle->active = 1;
    vehicle->is_ambulance = 0;
    vehicle->city_map = city_map;
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
