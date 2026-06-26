#include <stdlib.h>

#include "CityMap.h"
#include "Intersection.h"
#include "Road.h"

static const int HORIZONTAL_ROAD_ROWS[] = {4, 10, 16};
static const RoadType HORIZONTAL_ROAD_TYPES[] = {
    ROAD_ONE_WAY,
    ROAD_TWO_WAY,
    ROAD_TWO_WAY
};

static const int VERTICAL_ROAD_COLS[] = {8, 16, 24, 32};
static const RoadType VERTICAL_ROAD_TYPES[] = {
    ROAD_TWO_WAY,
    ROAD_ONE_WAY,
    ROAD_TWO_WAY,
    ROAD_TWO_WAY
};

#define HORIZONTAL_ROAD_COUNT 3
#define VERTICAL_ROAD_COUNT 4
#define TOTAL_ROAD_COUNT (HORIZONTAL_ROAD_COUNT + VERTICAL_ROAD_COUNT)
#define TOTAL_INTERSECTION_COUNT (HORIZONTAL_ROAD_COUNT * VERTICAL_ROAD_COUNT)

static void destroy_intersections(CityMap *city)
{
    if (!city || !city->intersections)
        return;

    for (int i = 0; i < city->intersection_count; i++)
    {
        if (city->intersections[i])
        {
            intersection_destroy(city->intersections[i]);
            free(city->intersections[i]);
        }
    }

    free(city->intersections);
    city->intersections = NULL;
}

static void destroy_cells(CityMap *city, int allocated_rows)
{
    if (!city || !city->cells)
        return;

    for (int row = 0; row < allocated_rows; row++)
    {
        if (city->cells[row])
        {
            for (int col = 0; col < city->cols; col++)
                cell_destroy(&city->cells[row][col]);

            free(city->cells[row]);
        }
    }

    free(city->cells);
    city->cells = NULL;
}

static void destroy_roads(CityMap *city)
{
    if (!city || !city->roads)
        return;

    for (int i = 0; i < city->road_count; i++)
        road_destroy(city->roads[i]);

    free(city->roads);
    city->roads = NULL;
}

static int allocate_cells(CityMap *city)
{
    if (!city)
        return 0;

    city->cells = malloc(city->rows * sizeof(Cell *));

    if (!city->cells)
        return 0;

    for (int row = 0; row < city->rows; row++)
    {
        city->cells[row] = malloc(city->cols * sizeof(Cell));

        if (!city->cells[row])
        {
            destroy_cells(city, row);
            return 0;
        }

        for (int col = 0; col < city->cols; col++)
            cell_init(&city->cells[row][col], row, col);
    }

    return 1;
}

static int initialize_roads_by_orientation(CityMap *city, RoadOrientation orientation)
{
    if (!city || !city->roads || !city->cells)
        return 0;

    int road_count = (orientation == ROAD_HORIZONTAL)
                         ? HORIZONTAL_ROAD_COUNT
                         : VERTICAL_ROAD_COUNT;
    int cell_count = (orientation == ROAD_HORIZONTAL) ? city->cols : city->rows;
    const int *positions = (orientation == ROAD_HORIZONTAL)
                               ? HORIZONTAL_ROAD_ROWS
                               : VERTICAL_ROAD_COLS;
    const RoadType *types = (orientation == ROAD_HORIZONTAL)
                                ? HORIZONTAL_ROAD_TYPES
                                : VERTICAL_ROAD_TYPES;
    int offset = (orientation == ROAD_HORIZONTAL) ? 0 : HORIZONTAL_ROAD_COUNT;

    for (int index = 0; index < road_count; index++)
    {
        city->roads[offset + index] = road_create(
            offset + index,
            orientation,
            types[index],
            cell_count
        );

        if (!city->roads[offset + index])
            return 0;

        int position = positions[index];

        for (int cell_index = 0; cell_index < cell_count; cell_index++)
        {
            int row = (orientation == ROAD_HORIZONTAL) ? position : cell_index;
            int col = (orientation == ROAD_HORIZONTAL) ? cell_index : position;

            city->roads[offset + index]->cells[cell_index] = &city->cells[row][col];
        }
    }

    return 1;
}

static int allocate_roads(CityMap *city)
{
    if (!city)
        return 0;

    city->road_count = TOTAL_ROAD_COUNT;
    city->roads = calloc(city->road_count, sizeof(Road *));

    if (!city->roads)
        return 0;

    if (!initialize_roads_by_orientation(city, ROAD_HORIZONTAL))
        return 0;

    if (!initialize_roads_by_orientation(city, ROAD_VERTICAL))
        return 0;

    return 1;
}

static int allocate_intersections(CityMap *city)
{
    if (!city)
        return 0;

    city->intersection_count = TOTAL_INTERSECTION_COUNT;
    city->intersections = calloc(city->intersection_count, sizeof(Intersection *));

    if (!city->intersections)
        return 0;

    int intersection_id = 0;

    for (int h = 0; h < HORIZONTAL_ROAD_COUNT; h++)
    {
        for (int v = 0; v < VERTICAL_ROAD_COUNT; v++)
        {
            int row = HORIZONTAL_ROAD_ROWS[h];
            int col = VERTICAL_ROAD_COLS[v];

            Intersection *intersection = intersection_create(
                intersection_id,
                row,
                col,
                city->roads[h],
                city->roads[HORIZONTAL_ROAD_COUNT + v]
            );

            if (!intersection)
                return 0;

            city->intersections[intersection_id] = intersection;
            city->roads[h]->intersections[col] = intersection;
            city->roads[HORIZONTAL_ROAD_COUNT + v]->intersections[row] = intersection;
            intersection_id++;
        }
    }

    return 1;
}
