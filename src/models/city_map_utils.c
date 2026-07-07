#include <stdlib.h>

#include "Intersection.h"
#include "CityMap.h"
#include "Road.h"
#include "Cell.h"

/* Linhas onde existem vias horizontais e seus sentidos */
static const int HORIZONTAL_ROAD_ROWS[] = {4, 10, 16};

static const RoadType HORIZONTAL_ROAD_TYPES[] = {
    ROAD_ONE_WAY,
    ROAD_TWO_WAY,
    ROAD_TWO_WAY
};

#define HORIZONTAL_ROAD_COUNT 3

/* Colunas onde existem vias verticais e seus sentidos */
static const int VERTICAL_ROAD_COLUMNS[] = {8, 16, 24, 32};

static const RoadType VERTICAL_ROAD_TYPES[] = {
    ROAD_TWO_WAY,
    ROAD_ONE_WAY,
    ROAD_TWO_WAY,
    ROAD_TWO_WAY
};

#define VERTICAL_ROAD_COUNT 4

#define TOTAL_ROAD_COUNT (HORIZONTAL_ROAD_COUNT + VERTICAL_ROAD_COUNT)
#define INTERSECTION_COUNT (HORIZONTAL_ROAD_COUNT * VERTICAL_ROAD_COUNT)

/* ── Funções de liberação ──────────────────────────────────────────────────── */

void release_intersections(CityMap *city_map)
{
    if (!city_map || !city_map->intersections) return;

    for (int i = 0; i < city_map->intersection_count; i++)
    {
        if (city_map->intersections[i])
        {
            intersection_destroy(city_map->intersections[i]);
            city_map->intersections[i] = NULL;
        }
    }

    free(city_map->intersections);
    city_map->intersections = NULL;
}

void release_cells(CityMap *city_map, int allocated_rows)
{
    if (!city_map || !city_map->cells) return;

    for (int row = 0; row < allocated_rows; row++)
    {
        if (city_map->cells[row])
        {
            for (int column = 0; column < city_map->columns; column++)
            {
                cell_destroy(&city_map->cells[row][column]);
            }

            free(city_map->cells[row]);
            city_map->cells[row] = NULL;
        }
    }

    free(city_map->cells);
    city_map->cells = NULL;
}

void release_roads(CityMap *city_map)
{
    if (!city_map || !city_map->roads) return;

    for (int i = 0; i < city_map->road_count; i++)
    {
        if (city_map->roads[i])
        {
            road_destroy(city_map->roads[i]);
            city_map->roads[i] = NULL;
        }
    }

    free(city_map->roads);
    city_map->roads = NULL;
}

void city_map_broadcast_intersections(CityMap *city_map)
{
    if (!city_map || !city_map->intersections)
        return;

    for (int i = 0; i < city_map->intersection_count; i++)
    {
        Intersection *intersection = city_map->intersections[i];

        if (!intersection)
            continue;

        pthread_mutex_lock(&intersection->mutex);
        pthread_cond_broadcast(&intersection->horizontal_cond);
        pthread_cond_broadcast(&intersection->vertical_cond);
        pthread_mutex_unlock(&intersection->mutex);
    }
}

/* ── Funções de alocação ───────────────────────────────────────────────────── */

int allocate_cells(CityMap *city_map)
{
    if (!city_map) return 0;

    city_map->cells = malloc(city_map->rows * sizeof(Cell *));
    if (!city_map->cells) return 0;

    /* Inicializa ponteiros a NULL para que release_cells seja seguro em falha parcial */
    for (int row = 0; row < city_map->rows; row++)
        city_map->cells[row] = NULL;

    for (int row = 0; row < city_map->rows; row++)
    {
        city_map->cells[row] = malloc(city_map->columns * sizeof(Cell));
        if (!city_map->cells[row])
        {
            /* Libera apenas as linhas já alocadas */
            release_cells(city_map, row);
            return 0;
        }

        for (int column = 0; column < city_map->columns; column++)
        {
            cell_init(&city_map->cells[row][column], row, column);
        }
    }

    return 1;
}

static int city_map_init_roads(CityMap *city_map, RoadDirection direction)
{
    if (!city_map || !city_map->roads || !city_map->cells) return 0;

    int road_count = (direction == ROAD_HORIZONTAL) ? HORIZONTAL_ROAD_COUNT : VERTICAL_ROAD_COUNT;
    int cell_count = (direction == ROAD_HORIZONTAL) ? city_map->columns : city_map->rows;

    const int *positions = (direction == ROAD_HORIZONTAL) ? HORIZONTAL_ROAD_ROWS : VERTICAL_ROAD_COLUMNS;
    const RoadType *types = (direction == ROAD_HORIZONTAL) ? HORIZONTAL_ROAD_TYPES : VERTICAL_ROAD_TYPES;

    int offset = (direction == ROAD_HORIZONTAL) ? 0 : HORIZONTAL_ROAD_COUNT;

    for (int index = 0; index < road_count; index++)
    {
        city_map->roads[offset + index] = road_create(
            offset + index,
            direction,
            types[index],
            cell_count
        );

        if (!city_map->roads[offset + index]) return 0;

        int position = positions[index];

        for (int cell = 0; cell < cell_count; cell++)
        {
            int row    = (direction == ROAD_HORIZONTAL) ? position : cell;
            int column = (direction == ROAD_HORIZONTAL) ? cell : position;

            city_map->roads[offset + index]->cells[cell] = &city_map->cells[row][column];
        }
    }

    return 1;
}

int allocate_roads(CityMap *city_map)
{
    if (!city_map) return 0;

    city_map->road_count = TOTAL_ROAD_COUNT;
    city_map->roads = calloc(city_map->road_count, sizeof(Road *));

    if (!city_map->roads) return 0;

    if (!city_map_init_roads(city_map, ROAD_HORIZONTAL))
        return 0;
    if (!city_map_init_roads(city_map, ROAD_VERTICAL))
        return 0;

    return 1;
}

int allocate_intersections(CityMap *city_map)
{
    if (!city_map) return 0;

    city_map->intersection_count = INTERSECTION_COUNT;
    city_map->intersections = calloc(city_map->intersection_count, sizeof(Intersection *));

    if (!city_map->intersections) return 0;

    int intersection_id = 0;

    for (int i = 0; i < HORIZONTAL_ROAD_COUNT; i++)
    {
        for (int j = 0; j < VERTICAL_ROAD_COUNT; j++)
        {
            Road *horizontal_road = city_map->roads[i];
            Road *vertical_road   = city_map->roads[HORIZONTAL_ROAD_COUNT + j];

            Intersection *intersection = intersection_create(
                intersection_id,
                HORIZONTAL_ROAD_ROWS[i],
                VERTICAL_ROAD_COLUMNS[j],
                horizontal_road,
                vertical_road
            );

            if (!intersection) return 0;

            city_map->intersections[intersection_id] = intersection;
            horizontal_road->intersections[VERTICAL_ROAD_COLUMNS[j]] = intersection;
            vertical_road->intersections[HORIZONTAL_ROAD_ROWS[i]]    = intersection;

            intersection_id++;
        }
    }
    return 1;
}
