#include <stdio.h>

#include "models/CityMap.h"
#include "models/Road.h"
#include "models/vehicle_thread.h"

static int failures = 0;

#define ASSERT_TRUE(condition)                                                   \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                        \
        {                                                                       \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                         \
        }                                                                       \
    } while (0)

static Position road_position(const Road *road, int index)
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

static int cell_contains_vehicle(Cell *cell, ThreadVehicle *vehicle)
{
    int contains;

    pthread_mutex_lock(&cell->mutex);
    contains = cell->occupied && cell->vehicle == (struct Vehicle *)vehicle;
    pthread_mutex_unlock(&cell->mutex);

    return contains;
}

static int cell_is_empty(Cell *cell)
{
    int empty;

    pthread_mutex_lock(&cell->mutex);
    empty = !cell->occupied && cell->vehicle == NULL;
    pthread_mutex_unlock(&cell->mutex);

    return empty;
}

static void init_vehicle_on_route(
    ThreadVehicle *vehicle,
    int id,
    Position route[],
    Direction direction,
    CityMap *city_map
)
{
    thread_vehicle_init(
        vehicle,
        id,
        VEHICLE_TYPE_CAR,
        route[0],
        direction,
        SPEED_FAST,
        route,
        2
    );
    thread_vehicle_attach_city_map(vehicle, city_map);
}

static void test_vehicle_moves_without_leaving_duplicate_occupancy(void)
{
    CityMap *city_map = city_map_create();
    Road *road = city_map->roads[0];
    Position route[2] = {road_position(road, 0), road_position(road, 1)};
    ThreadVehicle vehicle;
    Cell *origin = road_get_cell(road, 0);
    Cell *destination = road_get_cell(road, 1);

    init_vehicle_on_route(&vehicle, 1, route, DIRECTION_EAST, city_map);

    ASSERT_TRUE(thread_vehicle_place(&vehicle) == 1);
    ASSERT_TRUE(thread_vehicle_advance_one_step(&vehicle) == 1);

    ASSERT_TRUE(cell_is_empty(origin));
    ASSERT_TRUE(cell_contains_vehicle(destination, &vehicle));

    thread_vehicle_release(&vehicle);
    city_map_destroy(city_map);
}

static void test_vehicle_waits_when_next_cell_is_occupied(void)
{
    CityMap *city_map = city_map_create();
    Road *road = city_map->roads[0];
    Position front_route[2] = {road_position(road, 1), road_position(road, 2)};
    Position back_route[2] = {road_position(road, 0), road_position(road, 1)};
    ThreadVehicle front;
    ThreadVehicle back;
    Cell *back_origin = road_get_cell(road, 0);
    Cell *front_cell = road_get_cell(road, 1);

    init_vehicle_on_route(&front, 1, front_route, DIRECTION_EAST, city_map);
    init_vehicle_on_route(&back, 2, back_route, DIRECTION_EAST, city_map);

    ASSERT_TRUE(thread_vehicle_place(&front) == 1);
    ASSERT_TRUE(thread_vehicle_place(&back) == 1);

    ASSERT_TRUE(thread_vehicle_advance_one_step(&back) == 0);
    ASSERT_TRUE(back.state == VEHICLE_STATE_WAITING_CELL);
    ASSERT_TRUE(cell_contains_vehicle(back_origin, &back));
    ASSERT_TRUE(cell_contains_vehicle(front_cell, &front));

    thread_vehicle_release(&back);
    thread_vehicle_release(&front);
    city_map_destroy(city_map);
}

static void test_one_way_rejects_reverse_direction(void)
{
    CityMap *city_map = city_map_create();
    Road *one_way_road = city_map->roads[0];
    Position route[2] = {
        road_position(one_way_road, 1),
        road_position(one_way_road, 0)
    };
    ThreadVehicle vehicle;
    Cell *origin = road_get_cell(one_way_road, 1);
    Cell *destination = road_get_cell(one_way_road, 0);

    init_vehicle_on_route(&vehicle, 1, route, DIRECTION_WEST, city_map);

    ASSERT_TRUE(thread_vehicle_place(&vehicle) == 1);
    ASSERT_TRUE(thread_vehicle_advance_one_step(&vehicle) == 0);
    ASSERT_TRUE(cell_contains_vehicle(origin, &vehicle));
    ASSERT_TRUE(cell_is_empty(destination));

    thread_vehicle_release(&vehicle);
    city_map_destroy(city_map);
}

static void test_two_way_allows_reverse_direction(void)
{
    CityMap *city_map = city_map_create();
    Road *two_way_road = city_map->roads[1];
    Position route[2] = {
        road_position(two_way_road, 1),
        road_position(two_way_road, 0)
    };
    ThreadVehicle vehicle;
    Cell *origin = road_get_cell(two_way_road, 1);
    Cell *destination = road_get_cell(two_way_road, 0);

    init_vehicle_on_route(&vehicle, 1, route, DIRECTION_WEST, city_map);

    ASSERT_TRUE(thread_vehicle_place(&vehicle) == 1);
    ASSERT_TRUE(thread_vehicle_advance_one_step(&vehicle) == 1);
    ASSERT_TRUE(cell_is_empty(origin));
    ASSERT_TRUE(cell_contains_vehicle(destination, &vehicle));

    thread_vehicle_release(&vehicle);
    city_map_destroy(city_map);
}

static void test_vehicle_cannot_move_outside_map_bounds(void)
{
    CityMap *city_map = city_map_create();
    Road *road = city_map->roads[0];
    Position start = road_position(road, 0);
    Position route[2] = {
        start,
        {start.row, -1}
    };
    ThreadVehicle vehicle;
    Cell *origin = road_get_cell(road, 0);

    init_vehicle_on_route(&vehicle, 1, route, DIRECTION_WEST, city_map);

    ASSERT_TRUE(thread_vehicle_place(&vehicle) == 1);
    ASSERT_TRUE(thread_vehicle_advance_one_step(&vehicle) == 0);
    ASSERT_TRUE(cell_contains_vehicle(origin, &vehicle));

    thread_vehicle_release(&vehicle);
    city_map_destroy(city_map);
}

int main(void)
{
    test_vehicle_moves_without_leaving_duplicate_occupancy();
    test_vehicle_waits_when_next_cell_is_occupied();
    test_one_way_rejects_reverse_direction();
    test_two_way_allows_reverse_direction();
    test_vehicle_cannot_move_outside_map_bounds();

    if (failures)
    {
        fprintf(stderr, "%d vehicle invariant test(s) failed.\n", failures);
        return 1;
    }

    puts("Vehicle invariant tests passed.");
    return 0;
}
