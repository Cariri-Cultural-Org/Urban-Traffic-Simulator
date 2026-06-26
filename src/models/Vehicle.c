#include <string.h>

#include "Vehicle.h"

static VehicleDirection default_direction_for_road(const Road *road)
{
    if (!road)
        return VEHICLE_DIRECTION_RIGHT;

    return (road->direction == ROAD_HORIZONTAL)
               ? VEHICLE_DIRECTION_RIGHT
               : VEHICLE_DIRECTION_DOWN;
}

void vehicle_init(
    Vehicle *vehicle,
    int id,
    int velocity,
    Road *road,
    int road_cell_index,
    void *city_map
)
{
    Cell *cell = road_get_cell(road, road_cell_index);

    memset(vehicle, 0, sizeof(Vehicle));
    vehicle->id = id;
    vehicle->velocity = velocity;
    vehicle->current_road = road;
    vehicle->road_cell_index = road_cell_index;
    vehicle->row = cell ? cell->row : 0;
    vehicle->column = cell ? cell->column : 0;
    vehicle->direction = default_direction_for_road(road);
    vehicle->route[0] = road;
    vehicle->route_length = road ? 1 : 0;
    vehicle->route_index = 0;
    vehicle->active = 1;
    vehicle->city_map = city_map;
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
        if (route[i])
            vehicle->route[vehicle->route_length++] = route[i];
    }

    vehicle->route_index = 0;

    for (int i = 0; i < vehicle->route_length; i++)
    {
        if (vehicle->route[i] == vehicle->current_road)
        {
            vehicle->route_index = i;
            break;
        }
    }
}
