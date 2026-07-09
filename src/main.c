#include <stdio.h>
#include "models/SimulationOutput.h"
#include "models/CityMap.h"
#include "models/city_map_utils.h"
#include "models/GlobalClock.h"
#include "models/Road.h"
#include "models/vehicle_thread.h"

#define DEMO_ROUTE_LENGTH 40
#define DEMO_CAR_COUNT 15
#define DEMO_AMBULANCE_COUNT 3
#define DEMO_VEHICLE_COUNT (DEMO_CAR_COUNT + DEMO_AMBULANCE_COUNT)
#define DEMO_SIMULATION_TICKS 60
#define SIGNAL_INTERVAL_TICKS 3

typedef struct
{
    int road_index;
    int start_index;
    VehicleType type;
    Direction direction;
    Speed speed;
} DemoVehiclePlan;

static const DemoVehiclePlan DEMO_VEHICLE_PLANS[DEMO_VEHICLE_COUNT] = {
    {0, 0, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_SLOW},
    {0, 5, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_MEDIUM},
    {0, 10, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_FAST},
    {1, 0, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_SLOW},
    {1, 5, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_FAST},
    {1, 10, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_MEDIUM},
    {1, 15, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_SLOW},
    {2, 0, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_FAST},
    {2, 6, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_MEDIUM},
    {2, 12, VEHICLE_TYPE_CAR, DIRECTION_EAST, SPEED_SLOW},
    {3, 0, VEHICLE_TYPE_CAR, DIRECTION_SOUTH, SPEED_MEDIUM},
    {3, 5, VEHICLE_TYPE_CAR, DIRECTION_SOUTH, SPEED_SLOW},
    {4, 0, VEHICLE_TYPE_CAR, DIRECTION_SOUTH, SPEED_FAST},
    {5, 0, VEHICLE_TYPE_CAR, DIRECTION_SOUTH, SPEED_MEDIUM},
    {5, 5, VEHICLE_TYPE_CAR, DIRECTION_SOUTH, SPEED_SLOW},
    {6, 0, VEHICLE_TYPE_AMBULANCE, DIRECTION_SOUTH, SPEED_FAST},
    {4, 5, VEHICLE_TYPE_AMBULANCE, DIRECTION_SOUTH, SPEED_MEDIUM},
    {0, 15, VEHICLE_TYPE_AMBULANCE, DIRECTION_EAST, SPEED_FAST}
};

static Road *find_demo_road(CityMap *city_map, RoadDirection direction)
{
    if (!city_map)
        return NULL;

    for (int i = 0; i < city_map->road_count; i++)
    {
        Road *road = city_map->roads[i];

        if (road && road->direction == direction && road->cell_count > 0)
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

static int build_route_from_road(
    const Road *road,
    int start_index,
    Position route[],
    int route_size
)
{
    if (!road || !route || start_index < 0 || start_index >= road->cell_count)
        return 0;

    if (route_size > road->cell_count - start_index)
        route_size = road->cell_count - start_index;

    for (int i = 0; i < route_size; i++)
        route[i] = position_from_road_cell(road, start_index + i);

    return route_size;
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

static const char *vehicle_type_label(VehicleType type)
{
    return type == VEHICLE_TYPE_AMBULANCE ? "ambulance" : "car";
}

static const char *speed_label(Speed speed)
{
    switch (speed)
    {
    case SPEED_FAST:
        return "fast";
    case SPEED_MEDIUM:
        return "medium";
    case SPEED_SLOW:
        return "slow";
    default:
        return "unknown";
    }
}

static const char *road_direction_label(RoadDirection direction)
{
    return direction == ROAD_HORIZONTAL ? "horizontal" : "vertical";
}

int main(void)
{
    CityMap *city_map;
    Road *horizontal_road;
    Road *vertical_road;
    os_thread_t clock_thread;
    ThreadVehicle vehicles[DEMO_VEHICLE_COUNT];
    Position routes[DEMO_VEHICLE_COUNT][DEMO_ROUTE_LENGTH];
    int route_sizes[DEMO_VEHICLE_COUNT];
    int started_vehicles = 0;

    simulation_output_init();
    simulation_output_set_log_enabled(0);
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
        "Demo: %d cars + %d ambulances on horizontal road #%d and vertical road #%d\n",
        DEMO_CAR_COUNT,
        DEMO_AMBULANCE_COUNT,
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

    for (int i = 0; i < DEMO_VEHICLE_COUNT; i++)
    {
        const DemoVehiclePlan *plan = &DEMO_VEHICLE_PLANS[i];
        Road *vehicle_road = city_map->roads[plan->road_index];

        if (!vehicle_road)
        {
            simulation_output_log("ERROR: road unavailable for vehicle #%d.\n", i + 1);
            stop_global_clock();
            pthread_join(clock_thread, NULL);
            destroy_global_clock();
            city_map_destroy(city_map);
            simulation_output_destroy();
            return 1;
        }

        route_sizes[i] = build_route_from_road(
            vehicle_road,
            plan->start_index,
            routes[i],
            DEMO_ROUTE_LENGTH
        );

        if (route_sizes[i] <= 0)
        {
            simulation_output_log("ERROR: failed to build route for vehicle #%d.\n", i + 1);
            stop_global_clock();
            pthread_join(clock_thread, NULL);
            destroy_global_clock();
            city_map_destroy(city_map);
            simulation_output_destroy();
            return 1;
        }

        simulation_output_log(
            "[Plan] vehicle #%d: %s | road #%d (%s) | start=%d | route=%d cells | speed=%s (%d tick%s)\n",
            i + 1,
            vehicle_type_label(plan->type),
            vehicle_road->id,
            road_direction_label(vehicle_road->direction),
            plan->start_index,
            route_sizes[i],
            speed_label(plan->speed),
            (int)plan->speed,
            plan->speed == SPEED_FAST ? "" : "s"
        );

        thread_vehicle_init(
            &vehicles[i],
            i + 1,
            plan->type,
            routes[i][0],
            plan->direction,
            plan->speed,
            routes[i],
            route_sizes[i]
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
        city_map_broadcast_intersections(city_map);

        for (int i = 0; i < started_vehicles; i++)
            thread_vehicle_join(&vehicles[i]);

        pthread_join(clock_thread, NULL);
        destroy_global_clock();
        city_map_destroy(city_map);
        simulation_output_destroy();
        return 1;
    }

    int rendered_tick = global_clock_current_tick();
    int target_tick = rendered_tick + DEMO_SIMULATION_TICKS;

    while (rendered_tick < target_tick)
    {
        int observed_tick = rendered_tick;

        wait_next_tick(observed_tick);
        rendered_tick = global_clock_current_tick();

        if (rendered_tick % SIGNAL_INTERVAL_TICKS == 0)
            toggle_city_map_signals(city_map);

        printf("\033[H\033[J");
        fflush(stdout);

        if (!simulation_output_render_city_map(city_map, stdout, rendered_tick))
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
    pthread_join(clock_thread, NULL);

    destroy_global_clock();
    city_map_destroy(city_map);

    simulation_output_log("Cleanup completed. Leaving simulator.\n");
    simulation_output_destroy();
    return 0;
}
