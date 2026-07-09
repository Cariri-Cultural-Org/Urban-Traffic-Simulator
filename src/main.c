#include <stdio.h>
#include "models/SimulationOutput.h"
#include "models/CityMap.h"
#include "models/city_map_utils.h"
#include "models/GlobalClock.h"
#include "models/Road.h"
#include "models/vehicle_thread.h"

#define DEMO_ROUTE_LENGTH 20
#define DEMO_VEHICLE_COUNT 10
#define DEMO_SIMULATION_TICKS 30
#define SIGNAL_INTERVAL_TICKS 3

static Road *find_demo_road(CityMap *city_map, RoadDirection direction)
{
    if (!city_map)
        return NULL;

    for (int i = 0; i < city_map->road_count; i++)
    {
        Road *road = city_map->roads[i];

        if (road && road->direction == direction &&
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

static void toggle_city_map_signals(CityMap *city_map)
{
    if (!city_map || !city_map->intersections)
        return;

    for (int i = 0; i < city_map->intersection_count; i++)
    {
        if (city_map->intersections[i])
            intersection_toggle_signal(city_map->intersections[i]);
    }
}

int main(void)
{
    CityMap *city_map;
    Road *horizontal_road;
    Road *vertical_road;
    os_thread_t clock_thread;
    os_thread_t test_thread;
    ThreadVehicle vehicles[DEMO_VEHICLE_COUNT];
    Position routes[DEMO_VEHICLE_COUNT][DEMO_ROUTE_LENGTH];
    const Road *vehicle_roads[DEMO_VEHICLE_COUNT];
    const int route_starts[DEMO_VEHICLE_COUNT] = {0, 3, 6, 9, 12, 15, 1, 3, 5, 7};
    const VehicleType vehicle_types[DEMO_VEHICLE_COUNT] = {
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_CAR,
        VEHICLE_TYPE_AMBULANCE
    };
    const Direction vehicle_directions[DEMO_VEHICLE_COUNT] = {
        DIRECTION_EAST,
        DIRECTION_EAST,
        DIRECTION_EAST,
        DIRECTION_EAST,
        DIRECTION_EAST,
        DIRECTION_EAST,
        DIRECTION_SOUTH,
        DIRECTION_SOUTH,
        DIRECTION_SOUTH,
        DIRECTION_SOUTH
    };
    const Speed vehicle_speeds[DEMO_VEHICLE_COUNT] = {
        SPEED_SLOW,
        SPEED_FAST,
        SPEED_MEDIUM,
        SPEED_SLOW,
        SPEED_FAST,
        SPEED_MEDIUM,
        SPEED_SLOW,
        SPEED_MEDIUM,
        SPEED_FAST,
        SPEED_FAST
    };
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

    horizontal_road = find_demo_road(city_map, ROAD_HORIZONTAL);
    vertical_road = find_demo_road(city_map, ROAD_VERTICAL);
    if (!horizontal_road || !vertical_road)
    {
        simulation_output_log("ERROR: demo roads unavailable.\n");
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    simulation_output_log("Map: %d rows x %d columns\n",
                          city_map->rows, city_map->columns);
    simulation_output_log("Roads: %d | Intersections: %d\n",
                          city_map->road_count, city_map->intersection_count);
    simulation_output_log(
        "Demo: %d vehicles on horizontal road #%d and vertical road #%d\n",
        DEMO_VEHICLE_COUNT,
        horizontal_road->id,
        vertical_road->id
    );

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

    for (int i = 0; i < DEMO_VEHICLE_COUNT; i++)
    {
        vehicle_roads[i] = i < 6 ? horizontal_road : vertical_road;
        build_route_from_road(
            vehicle_roads[i],
            route_starts[i],
            routes[i],
            DEMO_ROUTE_LENGTH
        );

        thread_vehicle_init(
            &vehicles[i],
            i + 1,
            vehicle_types[i],
            routes[i][0],
            vehicle_directions[i],
            vehicle_speeds[i],
            routes[i],
            DEMO_ROUTE_LENGTH
        );
        thread_vehicle_attach_city_map(&vehicles[i], city_map);
    }

    for (int i = 0; i < DEMO_VEHICLE_COUNT; i++)
    {
        if (thread_vehicle_start(&vehicles[i]) == 0)
            started_vehicles++;
    }

    if (started_vehicles != DEMO_VEHICLE_COUNT)
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

    for (int i = 0; i < DEMO_SIMULATION_TICKS; i++)
    {
        int observed_tick = global_tick;

        wait_next_tick(observed_tick);
        if (global_tick % SIGNAL_INTERVAL_TICKS == 0)
            toggle_city_map_signals(city_map);

        // printf("\033[H\033[J");
        fflush(stdout);

        if (!simulation_output_render_city_map(city_map, stdout, global_tick))
        {
            simulation_output_log("ERROR: failed to render ASCII map.\n");
            break;
        }
    }

    simulation_output_log("Stopping simulation...\n");
    stop_global_clock();
    city_map_broadcast_intersections(city_map);

    for (int i = 0; i < started_vehicles; i++)
        thread_vehicle_join(&vehicles[i]);
    pthread_join(test_thread, NULL);
    pthread_join(clock_thread, NULL);

    destroy_global_clock();
    city_map_destroy(city_map);

    simulation_output_log("Cleanup completed. Leaving simulator.\n");
    simulation_output_destroy();
    return 0;
}
