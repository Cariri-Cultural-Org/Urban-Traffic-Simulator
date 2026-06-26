#ifndef ROAD_H
#define ROAD_H

#include "Cell.h"

typedef struct Intersection Intersection;

typedef enum
{
    ROAD_HORIZONTAL,
    ROAD_VERTICAL
} RoadOrientation;

typedef enum
{
    ROAD_ONE_WAY,
    ROAD_TWO_WAY
} RoadType;

typedef struct
{
    int id;
    RoadOrientation orientation;
    RoadType type;
    Cell **cells;
    Intersection **intersections;
    int cell_count;
} Road;

Road *road_create(int id, RoadOrientation orientation, RoadType type, int cell_count);
void road_destroy(Road *road);
Cell *road_get_cell(const Road *road, int index);
Intersection *road_get_intersection(const Road *road, int index);

#endif
