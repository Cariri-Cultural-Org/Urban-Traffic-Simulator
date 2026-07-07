#include <stdio.h>

#include "SimulationOutput.h"
#include "models/CityMap.h"
#include "models/GlobalClock.h"
#include "models/Road.h"
#include "vehicle_thread.h"

#define DEMO_ROUTE_LENGTH 10

static Road *find_demo_road(CityMap *city_map)
{
    if (!city_map)
        return NULL;

    for (int i = 0; i < city_map->road_count; i++)
    {
        Road *road = city_map->roads[i];

        if (road && road->direction == ROAD_HORIZONTAL &&
            road->cell_count >= DEMO_ROUTE_LENGTH)
        {
            return road;
        }
    }

    return NULL;
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

static void build_route_from_road(
    const Road *road,
    int start_index,
    Position route[],
    int route_size
)
{
    for (int i = 0; i < route_size; i++)
        route[i] = position_from_road_cell(road, start_index + i);
}

static void *clock_test_thread(void *arg)
{
    (void)arg;

    int observed_tick = global_tick;

    while (simulation_running)
    {
        wait_next_tick(observed_tick);

        if (!simulation_running)
            break;

        observed_tick = global_tick;
        simulation_output_log("[Test] waited tick %d and continued\n", observed_tick);
    }

    simulation_output_log("[Test] finished with global clock\n");
    return NULL;
}

int main(void)
{
    CityMap *city_map;
    Road *demo_road;
    os_thread_t clock_thread;
    os_thread_t test_thread;
    ThreadVehicle vehicles[2];
    Position slow_route[DEMO_ROUTE_LENGTH];
    Position fast_route[DEMO_ROUTE_LENGTH];
    int started_vehicles = 0;

    simulation_output_init();
    simulation_output_log("--- Starting the Urban Traffic Simulator ---\n");

    city_map = city_map_create();
    if (!city_map)
    {
        simulation_output_log("ERROR: failed to create CityMap.\n");
        simulation_output_destroy();
        return 1;
    }

    demo_road = find_demo_road(city_map);
    if (!demo_road)
    {
        simulation_output_log("ERROR: no horizontal road available for Increment 2 demo.\n");
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    simulation_output_log("Map: %d rows x %d columns\n",
                          city_map->rows, city_map->columns);
    simulation_output_log("Roads: %d | Intersections: %d\n",
                          city_map->road_count, city_map->intersection_count);
    simulation_output_log("Increment 2 demo: two cars sharing road #%d\n",
                          demo_road->id);

    init_global_clock();

    if (pthread_create(&clock_thread, NULL, thread_global_clock, NULL) != 0)
    {
        simulation_output_log("ERROR: failed to start global clock.\n");
        destroy_global_clock();
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    if (pthread_create(&test_thread, NULL, clock_test_thread, NULL) != 0)
    {
        simulation_output_log("ERROR: failed to start clock test thread.\n");
        stop_global_clock();
        pthread_join(clock_thread, NULL);
        destroy_global_clock();
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    build_route_from_road(demo_road, 1, slow_route, DEMO_ROUTE_LENGTH);
    build_route_from_road(demo_road, 0, fast_route, DEMO_ROUTE_LENGTH);

    thread_vehicle_init(
        &vehicles[0],
        1,
        VEHICLE_TYPE_CAR,
        slow_route[0],
        DIRECTION_EAST,
        SPEED_SLOW,
        slow_route,
        DEMO_ROUTE_LENGTH
    );
    thread_vehicle_attach_city_map(&vehicles[0], city_map);

    thread_vehicle_init(
        &vehicles[1],
        2,
        VEHICLE_TYPE_CAR,
        fast_route[0],
        DIRECTION_EAST,
        SPEED_FAST,
        fast_route,
        DEMO_ROUTE_LENGTH
    );
    thread_vehicle_attach_city_map(&vehicles[1], city_map);

    if (thread_vehicle_start(&vehicles[0]) == 0)
        started_vehicles++;

    if (thread_vehicle_start(&vehicles[1]) == 0)
        started_vehicles++;

    if (started_vehicles != 2)
    {
        simulation_output_log("ERROR: failed to start vehicle threads.\n");
        stop_global_clock();

        for (int i = 0; i < started_vehicles; i++)
            thread_vehicle_join(&vehicles[i]);

        pthread_join(test_thread, NULL);
        pthread_join(clock_thread, NULL);
        destroy_global_clock();
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    for (int i = 0; i < 12; i++)
    {
        int observed_tick = global_tick;

        wait_next_tick(observed_tick);

        if (!simulation_output_render_city_map(city_map, stdout, global_tick))
        {
            simulation_output_log("ERROR: failed to render ASCII map.\n");
            break;
        }
    }

    simulation_output_log("Stopping simulation...\n");
    stop_global_clock();

    thread_vehicle_join(&vehicles[0]);
    thread_vehicle_join(&vehicles[1]);
    pthread_join(test_thread, NULL);
    pthread_join(clock_thread, NULL);

    destroy_global_clock();
    city_map_destroy(city_map);

    simulation_output_log("Cleanup completed. Leaving simulator.\n");
    simulation_output_destroy();
    return 0;
}
