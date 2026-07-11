#ifndef SIMULATION_OUTPUT_H
#define SIMULATION_OUTPUT_H

#include <stdio.h>

#include "models/CityMap.h"

void simulation_output_init(void);
void simulation_output_destroy(void);
void simulation_output_set_log_enabled(int enabled);
int simulation_output_set_log_file(const char *path);
void simulation_output_log(const char *format, ...);
int simulation_output_render_city_map(CityMap *city_map, FILE *stream, int tick);

#endif /* SIMULATION_OUTPUT_H */
