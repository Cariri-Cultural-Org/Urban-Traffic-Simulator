#ifndef CITY_MAP_RENDERER_H
#define CITY_MAP_RENDERER_H

#include <stdio.h>

#include "Cell.h"
#include "CityMap.h"

typedef char (*CityMapVehicleSymbolResolver)(const Cell *cell, void *context);

typedef struct
{
    char empty_cell;
    char horizontal_road;
    char vertical_road;
    char intersection;
    char horizontal_signal;
    char vertical_signal;
    char occupied_cell;
    int show_tick;

    CityMapVehicleSymbolResolver vehicle_symbol_resolver;
    void *vehicle_symbol_context;
} CityMapAsciiRenderOptions;

extern const CityMapAsciiRenderOptions CITY_MAP_ASCII_RENDER_DEFAULT_OPTIONS;

int city_map_render_ascii(CityMap *city_map, FILE *stream, int tick);
int city_map_render_ascii_with_options(
    CityMap *city_map,
    FILE *stream,
    int tick,
    const CityMapAsciiRenderOptions *options
);

#endif /* CITY_MAP_RENDERER_H */
