#include <stdlib.h>

#include "city_map_utils.c"

CityMap *city_map_create(void)
{
    CityMap *city = calloc(1, sizeof(CityMap));

    if (!city)
        return NULL;

    city->rows = CITY_MAP_ROWS;
    city->cols = CITY_MAP_COLS;
    pthread_mutex_init(&city->state_mutex, NULL);

    if (!allocate_cells(city))
    {
        city_map_destroy(city);
        return NULL;
    }

    if (!allocate_roads(city))
    {
        city_map_destroy(city);
        return NULL;
    }

    if (!allocate_intersections(city))
    {
        city_map_destroy(city);
        return NULL;
    }

    return city;
}

void city_map_destroy(CityMap *city)
{
    if (!city)
        return;

    if (city->intersections)
        destroy_intersections(city);
    if (city->roads)
        destroy_roads(city);
    if (city->cells)
        destroy_cells(city, city->rows);

    pthread_mutex_destroy(&city->state_mutex);
    free(city);
}

Cell *city_map_get_cell(const CityMap *city, int row, int col)
{
    if (!city_map_position_is_valid(city, row, col))
        return NULL;

    return &city->cells[row][col];
}

Intersection *city_map_get_intersection(const CityMap *city, int row, int col)
{
    if (!city || !city->intersections)
        return NULL;

    for (int i = 0; i < city->intersection_count; i++)
    {
        Intersection *intersection = city->intersections[i];

        if (intersection && intersection->row == row && intersection->col == col)
            return intersection;
    }

    return NULL;
}

int city_map_position_is_valid(const CityMap *city, int row, int col)
{
    if (!city)
        return 0;

    return row >= 0 && row < city->rows && col >= 0 && col < city->cols;
}
