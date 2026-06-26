#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "models/AsciiElements.h"
#include "models/CityMap.h"
#include "models/GlobalClock.h"
#include "models/Vehicle.h"

#define TICKS_PER_SECOND 10
#define SIMULATION_MINUTES 5
#define SIMULATION_TICKS (SIMULATION_MINUTES * 60 * TICKS_PER_SECOND)
#define SIGNAL_PERIOD_TICKS 30
#define VEHICLE_COUNT 14
#define HORIZONTAL_ROUTE_COUNT 3
#define VERTICAL_ROUTE_OFFSET 3
#define VERTICAL_ROUTE_COUNT 4

static int is_road_cell(CityMap *city, const Cell *cell, RoadOrientation orientation)
{
    for (int road_index = 0; road_index < city->road_count; road_index++)
    {
        Road *road = city->roads[road_index];

        if (!road || road->orientation != orientation)
            continue;

        int index = (orientation == ROAD_HORIZONTAL) ? cell->col : cell->row;

        if (index >= 0 && index < road->cell_count && road->cells[index] == cell)
            return 1;
    }

    return 0;
}

static char signal_marker_for_intersection(Intersection *intersection)
{
    char marker = ASCII_INTERSECTION;

    pthread_mutex_lock(&intersection->mutex);
    marker = (intersection->green_orientation == ROAD_HORIZONTAL)
                 ? ASCII_GREEN_HORIZONTAL
                 : ASCII_GREEN_VERTICAL;
    pthread_mutex_unlock(&intersection->mutex);

    return marker;
}

static char cell_marker(CityMap *city, Cell *cell)
{
    pthread_mutex_lock(&cell->mutex);

    if (cell->occupied && cell->vehicle)
    {
        char marker = cell->vehicle->is_ambulance ? ASCII_AMBULANCE : ASCII_CAR;
        pthread_mutex_unlock(&cell->mutex);
        return marker;
    }

    pthread_mutex_unlock(&cell->mutex);

    Intersection *intersection = city_map_get_intersection(city, cell->row, cell->col);

    if (intersection)
        return signal_marker_for_intersection(intersection);

    int horizontal = is_road_cell(city, cell, ROAD_HORIZONTAL);
    int vertical = is_road_cell(city, cell, ROAD_VERTICAL);

    if (horizontal && vertical)
        return ASCII_INTERSECTION;
    if (horizontal)
        return ASCII_HORIZONTAL_ROAD;
    if (vertical)
        return ASCII_VERTICAL_ROAD;

    return ASCII_EMPTY;
}

static void render_city(CityMap *city, int tick)
{
    printf("\033[H\033[J");
    printf("Urban Traffic Simulator MVP | tick %04d/%d\n", tick, SIMULATION_TICKS);
    printf("Legend: C car | A ambulance | H horizontal green | V vertical green\n\n");

    pthread_mutex_lock(&city->state_mutex);

    for (int row = 0; row < city->rows; row++)
    {
        for (int col = 0; col < city->cols; col++)
            putchar(cell_marker(city, &city->cells[row][col]));

        putchar('\n');
    }

    pthread_mutex_unlock(&city->state_mutex);
    fflush(stdout);
}

static void toggle_all_signals(CityMap *city)
{
    for (int i = 0; i < city->intersection_count; i++)
        intersection_toggle_signal(city->intersections[i]);
}

static void wake_all_intersections(CityMap *city)
{
    for (int i = 0; i < city->intersection_count; i++)
        intersection_wake_all(city->intersections[i]);
}

static int direction_moves_forward(VehicleDirection direction)
{
    return direction == VEHICLE_DIRECTION_RIGHT || direction == VEHICLE_DIRECTION_DOWN;
}

static int road_fixed_position(const Road *road)
{
    Cell *first_cell = road_get_cell(road, 0);

    if (!first_cell)
        return -1;

    return (road->orientation == ROAD_HORIZONTAL) ? first_cell->row : first_cell->col;
}

static Road *find_next_crossing_road(
    CityMap *city,
    Road *road,
    int start_index,
    VehicleDirection direction
)
{
    Road *fallback = NULL;
    Road *best = NULL;
    int best_distance = CITY_MAP_ROWS + CITY_MAP_COLS;
    int moves_forward = direction_moves_forward(direction);

    for (int i = 0; i < city->road_count; i++)
    {
        Road *candidate = city->roads[i];

        if (!candidate || candidate->orientation == road->orientation)
            continue;

        if (!fallback)
            fallback = candidate;

        int crossing_index = road_fixed_position(candidate);
        int distance = moves_forward
                           ? crossing_index - start_index
                           : start_index - crossing_index;

        if (distance > 0 && distance < best_distance)
        {
            best = candidate;
            best_distance = distance;
        }
    }

    return best ? best : fallback;
}

static void initialize_vehicles(CityMap *city, Vehicle vehicles[VEHICLE_COUNT])
{
    const int speeds[] = {
        VEHICLE_SPEED_FAST,
        VEHICLE_SPEED_MEDIUM,
        VEHICLE_SPEED_SLOW,
        VEHICLE_SPEED_FAST
    };

    for (int i = 0; i < VEHICLE_COUNT; i++)
    {
        Road *road = city->roads[i % city->road_count];
        int start_index = (i * 7 + road->id * 3) % road->cell_count;

        vehicle_init(
            &vehicles[i],
            i + 1,
            speeds[i % 4],
            road,
            start_index,
            city
        );

        Road *second_road = find_next_crossing_road(
            city,
            road,
            start_index,
            vehicles[i].direction
        );
        Road *route[] = {
            road,
            second_road,
            (road->orientation == ROAD_HORIZONTAL)
                ? city->roads[(i + 1) % HORIZONTAL_ROUTE_COUNT]
                : city->roads[VERTICAL_ROUTE_OFFSET + ((i + 1) % VERTICAL_ROUTE_COUNT)]
        };

        vehicle_set_route(&vehicles[i], route, 3);
    }

    vehicle_set_ambulance(&vehicles[0], 1);
    vehicles[0].speed = VEHICLE_SPEED_FAST;
}

static int start_vehicle_threads(Vehicle vehicles[VEHICLE_COUNT])
{
    for (int i = 0; i < VEHICLE_COUNT; i++)
    {
        if (pthread_create(&vehicles[i].thread, NULL, vehicle_thread, &vehicles[i]) != 0)
            return 0;
    }

    return 1;
}

static void join_vehicle_threads(Vehicle vehicles[VEHICLE_COUNT])
{
    for (int i = 0; i < VEHICLE_COUNT; i++)
        pthread_join(vehicles[i].thread, NULL);
}

int main(void)
{
    CityMap *city = city_map_create();

    if (!city)
    {
        fprintf(stderr, "Failed to create the city map.\n");
        return EXIT_FAILURE;
    }

    Vehicle vehicles[VEHICLE_COUNT];
    pthread_t clock;

    initialize_vehicles(city, vehicles);
    clock_init();

    if (pthread_create(&clock, NULL, clock_thread, NULL) != 0)
    {
        fprintf(stderr, "Failed to start the global clock thread.\n");
        clock_destroy();
        city_map_destroy(city);
        return EXIT_FAILURE;
    }

    if (!start_vehicle_threads(vehicles))
    {
        fprintf(stderr, "Failed to start a vehicle thread.\n");
        simulation_stop();
        wake_all_intersections(city);
        pthread_join(clock, NULL);
        clock_destroy();
        city_map_destroy(city);
        return EXIT_FAILURE;
    }

    int last_tick = clock_get_tick();

    while (simulation_is_running())
    {
        wait_next_tick(last_tick);
        int current_tick = clock_get_tick();

        if (current_tick == last_tick)
            continue;

        last_tick = current_tick;

        if (current_tick % SIGNAL_PERIOD_TICKS == 0)
            toggle_all_signals(city);

        render_city(city, current_tick);

        if (current_tick >= SIMULATION_TICKS)
            simulation_stop();
    }

    wake_all_intersections(city);
    join_vehicle_threads(vehicles);
    pthread_join(clock, NULL);

    city_map_destroy(city);
    clock_destroy();

    printf("\nSimulation finished cleanly.\n");
    return EXIT_SUCCESS;
}
