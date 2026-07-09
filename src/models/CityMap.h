#ifndef CITY_MAP_H
#define CITY_MAP_H

#include "Cell.h"
#include "Road.h"
#include "Intersection.h"

#define CITY_MAP_ROWS 20
#define CITY_MAP_COLUMNS 40

#define CITY_MAP_MIN_INTERSECTIONS 8

typedef struct
{
    int rows;
    int columns;
    pthread_mutex_t state_mutex;

    Cell **cells;
    Road **roads;
    int road_count;

    Intersection **intersections;
    int intersection_count;
} CityMap;

CityMap *city_map_create(void);
void city_map_destroy(CityMap *city_map);
Cell *city_map_get_cell(const CityMap *city_map, int row, int column);
Intersection *city_map_get_intersection(const CityMap *city_map, int row, int column);
int city_map_is_valid_position(const CityMap *city_map, int row, int column);

#endif
