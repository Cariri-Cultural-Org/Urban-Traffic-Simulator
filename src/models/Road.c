#include <stdlib.h>

#include "Intersection.h"
#include "Road.h"

Road *road_create(int id, RoadOrientation orientation, RoadType type, int cell_count)
{
    Road *road = malloc(sizeof(Road));

    if (!road)
        return NULL;

    road->id = id;
    road->orientation = orientation;
    road->type = type;
    road->cell_count = cell_count;
    road->cells = calloc(cell_count, sizeof(Cell *));

    if (!road->cells)
    {
        free(road);
        return NULL;
    }

    road->intersections = calloc(cell_count, sizeof(Intersection *));

    if (!road->intersections)
    {
        free(road->cells);
        free(road);
        return NULL;
    }

    return road;
}

void road_destroy(Road *road)
{
    if (!road)
        return;

    free(road->intersections);
    free(road->cells);
    free(road);
}

Cell *road_get_cell(const Road *road, int index)
{
    if (!road || index < 0 || index >= road->cell_count)
        return NULL;

    return road->cells[index];
}

Intersection *road_get_intersection(const Road *road, int index)
{
    if (!road || index < 0 || index >= road->cell_count)
        return NULL;

    return road->intersections[index];
}
