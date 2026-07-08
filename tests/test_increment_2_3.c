#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "models/vehicle_thread.h"
#include "models/CityMap.h"
#include "models/GlobalClock.h"
#include "models/Intersection.h"
#include "models/Road.h"

#define TEST_TIMEOUT_MS 1000
#define TEST_POLL_INTERVAL_US 1000

typedef int (*test_predicate_t)(void *context);

typedef struct
{
    ThreadVehicle *vehicle;
    VehicleState state;
} VehicleStateExpectation;

typedef struct
{
    Cell *cell;
} CellExpectation;

static void advance_test_tick(void)
{
    pthread_mutex_lock(&clock_mutex);
    global_tick++;
    pthread_cond_broadcast(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);
}

static int wait_until(test_predicate_t predicate, void *context, int timeout_ms)
{
    int elapsed_us = 0;
    int timeout_us = timeout_ms * 1000;

    while (elapsed_us <= timeout_us)
    {
        if (predicate(context))
            return 1;

        usleep(TEST_POLL_INTERVAL_US);
        elapsed_us += TEST_POLL_INTERVAL_US;
    }

    return 0;
}

static Road *find_horizontal_test_road(CityMap *city_map)
{
    if (!city_map)
        return NULL;

    for (int i = 0; i < city_map->road_count; i++)
    {
        Road *road = city_map->roads[i];

        if (road && road->direction == ROAD_HORIZONTAL && road->cell_count > 8)
            return road;
    }

    return NULL;
}

static int find_first_intersection_index(const Road *road)
{
    if (!road)
        return -1;

    for (int i = 1; i < road->cell_count; i++)
    {
        if (road_get_intersection(road, i))
            return i;
    }

    return -1;
}

static Position position_from_road_cell(const Road *road, int index)
{
    Cell *cell = road_get_cell(road, index);
    Position position = {0, 0};

    if (cell)
    {
        position.row = cell->row;
        position.column = cell->column;
    }

    return position;
}

static int vehicle_has_state(void *context)
{
    VehicleStateExpectation *expectation = context;

    return expectation->vehicle->state == expectation->state;
}

static int cell_is_occupied(void *context)
{
    CellExpectation *expectation = context;
    int occupied;

    pthread_mutex_lock(&expectation->cell->mutex);
    occupied = expectation->cell->occupied;
    pthread_mutex_unlock(&expectation->cell->mutex);

    return occupied;
}

static int cell_has_symbol(Cell *cell, char symbol)
{
    int result;

    pthread_mutex_lock(&cell->mutex);
    result = cell->occupied && cell->occupant_symbol == symbol;
    pthread_mutex_unlock(&cell->mutex);

    return result;
}

static int positions_are_equal(Position left, Position right)
{
    return left.row == right.row && left.column == right.column;
}

static int cell_mutex_is_available(Cell *cell)
{
    int result;

    result = pthread_mutex_trylock(&cell->mutex);
    if (result != 0)
        return 0;

    pthread_mutex_unlock(&cell->mutex);
    return 1;
}

static void test_rear_vehicle_waits_behind_occupied_cell(void)
{
    CityMap *city_map = city_map_create();
    Road *road;
    Cell *rear_cell;
    Cell *front_cell;
    Cell *front_next_cell;
    ThreadVehicle front_vehicle;
    ThreadVehicle rear_vehicle;
    Position rear_route[2];
    Position front_route[2];
    CellExpectation rear_cell_expectation;
    CellExpectation front_cell_expectation;
    VehicleStateExpectation waiting_cell_expectation;

    assert(city_map != NULL && "CityMap should be created");

    init_global_clock();
    road = find_horizontal_test_road(city_map);
    assert(road != NULL && "a horizontal road should exist");

    rear_cell = road_get_cell(road, 0);
    front_cell = road_get_cell(road, 1);
    front_next_cell = road_get_cell(road, 2);
    assert((rear_cell != NULL && front_cell != NULL &&
            front_next_cell != NULL) &&
           "test cells should exist");

    rear_route[0] = position_from_road_cell(road, 0);
    rear_route[1] = position_from_road_cell(road, 1);
    front_route[0] = position_from_road_cell(road, 1);
    front_route[1] = position_from_road_cell(road, 2);

    thread_vehicle_init(&front_vehicle, 101, VEHICLE_TYPE_CAR, front_route[0],
                        DIRECTION_EAST, SPEED_SLOW, front_route, 2);
    thread_vehicle_attach_city_map(&front_vehicle, city_map);

    assert(thread_vehicle_start(&front_vehicle) == 0 &&
           "front vehicle thread should start");

    front_cell_expectation.cell = front_cell;
    assert(wait_until(cell_is_occupied, &front_cell_expectation,
                      TEST_TIMEOUT_MS) &&
           "front vehicle should occupy the cell ahead");

    thread_vehicle_init(&rear_vehicle, 102, VEHICLE_TYPE_CAR, rear_route[0],
                        DIRECTION_EAST, SPEED_FAST, rear_route, 2);
    thread_vehicle_attach_city_map(&rear_vehicle, city_map);

    assert(thread_vehicle_start(&rear_vehicle) == 0 &&
           "rear vehicle thread should start");

    rear_cell_expectation.cell = rear_cell;
    assert(wait_until(cell_is_occupied, &rear_cell_expectation,
                      TEST_TIMEOUT_MS) &&
           "rear vehicle should occupy its initial cell");

    advance_test_tick();

    waiting_cell_expectation.vehicle = &rear_vehicle;
    waiting_cell_expectation.state = VEHICLE_STATE_WAITING_CELL;
    assert(wait_until(vehicle_has_state, &waiting_cell_expectation,
                      TEST_TIMEOUT_MS) &&
           "rear vehicle should wait because the next cell is occupied");

    assert(positions_are_equal(rear_vehicle.position, rear_route[0]) &&
           "rear vehicle should remain behind the occupied cell");
    assert(cell_has_symbol(front_cell, 'C') &&
           "front cell should remain occupied by the front vehicle");
    assert(cell_has_symbol(rear_cell, 'C') &&
           "rear cell should remain occupied by the rear vehicle");

    stop_global_clock();
    thread_vehicle_join(&rear_vehicle);
    thread_vehicle_join(&front_vehicle);
    destroy_global_clock();
    city_map_destroy(city_map);
}

static void test_signal_wait_does_not_hold_cell_mutex(void)
{
    CityMap *city_map = city_map_create();
    Road *road;
    int intersection_index;
    int origin_index;
    Cell *origin_cell;
    Cell *destination_cell;
    Intersection *intersection;
    ThreadVehicle vehicle;
    Position route[2];
    CellExpectation origin_cell_expectation;
    VehicleStateExpectation waiting_signal_expectation;
    VehicleStateExpectation finished_expectation;

    assert(city_map != NULL && "CityMap should be created");

    init_global_clock();
    road = find_horizontal_test_road(city_map);
    assert(road != NULL && "a horizontal road should exist");

    intersection_index = find_first_intersection_index(road);
    assert(intersection_index > 0 &&
           "road should have an intersection after one cell");

    origin_index = intersection_index - 1;
    origin_cell = road_get_cell(road, origin_index);
    destination_cell = road_get_cell(road, intersection_index);
    intersection = road_get_intersection(road, intersection_index);
    assert((origin_cell != NULL && destination_cell != NULL &&
            intersection != NULL) &&
           "signal wait scenario should have origin, destination and intersection");

    pthread_mutex_lock(&intersection->mutex);
    intersection->green_direction = ROAD_VERTICAL;
    intersection->previous_green_direction = ROAD_VERTICAL;
    pthread_mutex_unlock(&intersection->mutex);

    route[0] = position_from_road_cell(road, origin_index);
    route[1] = position_from_road_cell(road, intersection_index);

    thread_vehicle_init(&vehicle, 201, VEHICLE_TYPE_CAR, route[0],
                        DIRECTION_EAST, SPEED_FAST, route, 2);
    thread_vehicle_attach_city_map(&vehicle, city_map);

    assert(thread_vehicle_start(&vehicle) == 0 &&
           "vehicle thread should start");

    origin_cell_expectation.cell = origin_cell;
    assert(wait_until(cell_is_occupied, &origin_cell_expectation,
                      TEST_TIMEOUT_MS) &&
           "vehicle should occupy the cell before the red signal");

    advance_test_tick();

    waiting_signal_expectation.vehicle = &vehicle;
    waiting_signal_expectation.state = VEHICLE_STATE_WAITING_SIGNAL;
    assert(wait_until(vehicle_has_state, &waiting_signal_expectation,
                      TEST_TIMEOUT_MS) &&
           "vehicle should wait at the red signal");

    assert(cell_mutex_is_available(origin_cell) &&
           "origin cell mutex should not be held while waiting for signal");
    assert(cell_mutex_is_available(destination_cell) &&
           "destination cell mutex should not be held while waiting for signal");

    intersection_toggle_signal(intersection);

    finished_expectation.vehicle = &vehicle;
    finished_expectation.state = VEHICLE_STATE_FINISHED;
    assert(wait_until(vehicle_has_state, &finished_expectation,
                      TEST_TIMEOUT_MS) &&
           "vehicle should finish after the signal turns green");

    thread_vehicle_join(&vehicle);
    stop_global_clock();
    destroy_global_clock();
    city_map_destroy(city_map);
}

int main(void)
{
    test_rear_vehicle_waits_behind_occupied_cell();
    printf("[PASS] T2-C: rear vehicle waits behind an occupied cell\n");

    test_signal_wait_does_not_hold_cell_mutex();
    printf("[PASS] S3-D: signal wait does not hold cell mutexes\n");

    return 0;
}
