#include "CityMapRenderer.h"

#include <stdlib.h>

#include "Road.h"

const CityMapAsciiRenderOptions CITY_MAP_ASCII_RENDER_DEFAULT_OPTIONS = {
    ' ',
    '-',
    '|',
    '+',
    'H',
    'V',
    'C',
    1,
    NULL,
    NULL
};

static size_t grid_index(const CityMap *city_map, int row, int column)
{
    return (size_t)row * (size_t)city_map->columns + (size_t)column;
}

static char road_symbol_for_direction(
    RoadDirection direction,
    const CityMapAsciiRenderOptions *options
)
{
    return direction == ROAD_HORIZONTAL
               ? options->horizontal_road
               : options->vertical_road;
}

static void mark_road_cell(
    char *grid,
    const CityMap *city_map,
    const Cell *cell,
    RoadDirection direction,
    const CityMapAsciiRenderOptions *options
)
{
    char *slot;
    char road_symbol;

    if (!cell || !city_map_is_valid_position(city_map, cell->row, cell->column))
        return;

    slot = &grid[grid_index(city_map, cell->row, cell->column)];
    road_symbol = road_symbol_for_direction(direction, options);

    if (*slot == options->empty_cell)
    {
        *slot = road_symbol;
        return;
    }

    if (*slot != road_symbol)
        *slot = options->intersection;
}

static void mark_roads(
    char *grid,
    const CityMap *city_map,
    const CityMapAsciiRenderOptions *options
)
{
    for (int road_index = 0; road_index < city_map->road_count; road_index++)
    {
        Road *road = city_map->roads[road_index];

        if (!road)
            continue;

        for (int cell_index = 0; cell_index < road->cell_count; cell_index++)
        {
            mark_road_cell(
                grid,
                city_map,
                road_get_cell(road, cell_index),
                road->direction,
                options
            );
        }
    }
}

static void mark_intersections(
    char *grid,
    const CityMap *city_map,
    const CityMapAsciiRenderOptions *options
)
{
    for (int i = 0; i < city_map->intersection_count; i++)
    {
        Intersection *intersection = city_map->intersections[i];

        if (!intersection ||
            !city_map_is_valid_position(city_map, intersection->row, intersection->column))
        {
            continue;
        }

        pthread_mutex_lock(&intersection->mutex);
        grid[grid_index(city_map, intersection->row, intersection->column)] =
            intersection->green_direction == ROAD_HORIZONTAL
                ? options->horizontal_signal
                : options->vertical_signal;
        pthread_mutex_unlock(&intersection->mutex);
    }
}

static char occupied_symbol(
    const Cell *cell,
    const CityMapAsciiRenderOptions *options
)
{
    if (options->vehicle_symbol_resolver)
        return options->vehicle_symbol_resolver(cell, options->vehicle_symbol_context);

    return cell->occupant_symbol ? cell->occupant_symbol : options->occupied_cell;
}

static void overlay_occupied_cells(
    char *grid,
    const CityMap *city_map,
    const CityMapAsciiRenderOptions *options
)
{
    pthread_mutex_lock((pthread_mutex_t *)&city_map->state_mutex);

    for (int row = 0; row < city_map->rows; row++)
    {
        for (int column = 0; column < city_map->columns; column++)
        {
            Cell *cell = city_map_get_cell(city_map, row, column);
            int occupied;

            if (!cell)
                continue;

            pthread_mutex_lock(&cell->mutex);
            occupied = cell->occupied;

            if (occupied)
                grid[grid_index(city_map, row, column)] = occupied_symbol(cell, options);

            pthread_mutex_unlock(&cell->mutex);
        }
    }

    pthread_mutex_unlock((pthread_mutex_t *)&city_map->state_mutex);
}

static int write_frame(
    const char *grid,
    const CityMap *city_map,
    FILE *stream,
    int tick,
    const CityMapAsciiRenderOptions *options
)
{
    if (options->show_tick &&
        fprintf(
            stream,
            "Tick %d | H=sinal horizontal | V=sinal vertical | C=carro | A=ambulancia\n",
            tick
        ) < 0)
    {
        return 0;
    }

    for (int row = 0; row < city_map->rows; row++)
    {
        size_t offset = grid_index(city_map, row, 0);

        if (fwrite(&grid[offset], sizeof(char), (size_t)city_map->columns, stream) !=
            (size_t)city_map->columns)
        {
            return 0;
        }

        if (fputc('\n', stream) == EOF)
            return 0;
    }

    if (fputc('\n', stream) == EOF)
        return 0;

    return fflush(stream) == 0;
}

int city_map_render_ascii(const CityMap *city_map, FILE *stream, int tick)
{
    return city_map_render_ascii_with_options(
        city_map,
        stream,
        tick,
        &CITY_MAP_ASCII_RENDER_DEFAULT_OPTIONS
    );
}

int city_map_render_ascii_with_options(
    const CityMap *city_map,
    FILE *stream,
    int tick,
    const CityMapAsciiRenderOptions *options
)
{
    size_t cell_count;
    char *grid;
    int result;

    if (!city_map || !stream)
        return 0;

    if (!options)
        options = &CITY_MAP_ASCII_RENDER_DEFAULT_OPTIONS;

    if (city_map->rows <= 0 || city_map->columns <= 0)
        return 0;

    if ((size_t)city_map->rows > ((size_t)-1) / (size_t)city_map->columns)
        return 0;

    cell_count = (size_t)city_map->rows * (size_t)city_map->columns;
    grid = malloc((size_t)cell_count * sizeof(char));

    if (!grid)
        return 0;

    for (size_t i = 0; i < cell_count; i++)
        grid[i] = options->empty_cell;

    mark_roads(grid, city_map, options);
    mark_intersections(grid, city_map, options);
    overlay_occupied_cells(grid, city_map, options);

    result = write_frame(grid, city_map, stream, tick, options);

    free(grid);
    return result;
}
