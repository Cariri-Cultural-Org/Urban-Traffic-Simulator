#include <stdlib.h>
#include "CityMap.h"
#include "city_map_utils.h"

CityMap *city_map_create(void)
{
    CityMap *city_map = calloc(1, sizeof(CityMap));
    if (!city_map) return NULL;

    city_map->rows    = CITY_MAP_ROWS;
    city_map->columns = CITY_MAP_COLUMNS;
    pthread_mutex_init(&city_map->state_mutex, NULL);

    if (!allocate_cells(city_map))
    {
        city_map_destroy(city_map);
        return NULL;
    }

    if (!allocate_roads(city_map))
    {
        city_map_destroy(city_map);
        return NULL;
    }

    if (!allocate_intersections(city_map))
    {
        city_map_destroy(city_map);
        return NULL;
    }

    return city_map;
}

void city_map_destroy(CityMap *city_map)
{
    if (!city_map) return;
    if (city_map->intersections) release_intersections(city_map);
    if (city_map->roads)         release_roads(city_map);
    if (city_map->cells)         release_cells(city_map, city_map->rows);

    pthread_mutex_destroy(&city_map->state_mutex);
    free(city_map);
}

Cell *city_map_get_cell(const CityMap *city_map, int row, int column)
{
    if (!city_map_is_valid_position(city_map, row, column))
        return NULL;

    return &city_map->cells[row][column];
}

Intersection *city_map_get_intersection(const CityMap *city_map, int row, int column)
{
    if (!city_map || !city_map->intersections)
        return NULL;

    for (int i = 0; i < city_map->intersection_count; i++)
    {
        Intersection *intersection = city_map->intersections[i];

        if (intersection && intersection->row == row && intersection->column == column)
            return intersection;
    }

    return NULL;
}

int city_map_is_valid_position(const CityMap *city_map, int row, int column)
{
    if (!city_map)
        return 0;

    return row >= 0 && row < city_map->rows &&
           column >= 0 && column < city_map->columns;
}

void city_map_lock_state(CityMap *city_map)
{
    if (!city_map)
        return;

    pthread_mutex_lock(&city_map->state_mutex);
}

void city_map_unlock_state(CityMap *city_map)
{
    if (!city_map)
        return;

    pthread_mutex_unlock(&city_map->state_mutex);
}
