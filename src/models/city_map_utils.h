#ifndef CITY_MAP_UTILS_H
#define CITY_MAP_UTILS_H

/*
 * Funções utilitárias internas do CityMap.
 * Incluído apenas por CityMap.c — não use em outros módulos.
 */

#include "CityMap.h"

/* Alocação */
int allocate_cells(CityMap *city_map);
int allocate_roads(CityMap *city_map);
int allocate_intersections(CityMap *city_map);

/* Liberação — recebe o número de linhas realmente alocadas
   para que release_cells funcione corretamente em falhas parciais */
void release_cells(CityMap *city_map, int allocated_rows);
void release_roads(CityMap *city_map);
void release_intersections(CityMap *city_map);

/* Wake-up final para encerrar threads bloqueadas em semáforos */
void city_map_broadcast_intersections(CityMap *city_map);

#endif /* CITY_MAP_UTILS_H */
