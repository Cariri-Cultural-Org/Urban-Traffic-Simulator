#ifndef CITY_MAP_H
#define CITY_MAP_H

#include <pthread.h>

#include "Cell.h"
#include "Intersection.h"
#include "Road.h"

#define CITY_MAP_ROWS 20
#define CITY_MAP_COLS 40
#define CITY_MAP_MIN_INTERSECTIONS 8

typedef struct
{
    int rows;
    int cols;

    Cell **cells;
    Road **roads;
    int road_count;

    Intersection **intersections;
    int intersection_count;

    pthread_mutex_t state_mutex;
} CityMap;

CityMap *city_map_create(void);
void city_map_destroy(CityMap *city);
Cell *city_map_get_cell(const CityMap *city, int row, int col);
Intersection *city_map_get_intersection(const CityMap *city, int row, int col);
int city_map_position_is_valid(const CityMap *city, int row, int col);

#endif
