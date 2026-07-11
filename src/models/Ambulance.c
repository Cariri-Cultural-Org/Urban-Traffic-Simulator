#include <stdlib.h>
#include <stdbool.h>
#include "Ambulance.h"
#include "GlobalClock.h"
#include "CityMap.h"
#include "Intersection.h"

static int ambulance_place_initially(Vehicle *ambulance)
{
    while (simulation_running && ambulance->active)
    {
        if (vehicle_place_on_road(
                ambulance,
                ambulance->current_road,
                ambulance->road_cell_index
            ))
        {
            return 1;
        }

        wait_next_tick(global_clock_current_tick());
    }

    return 0;
}

#ifdef _WIN32
DWORD WINAPI thread_ambulance(LPVOID arg)
#else
void *thread_ambulance(void *arg)
#endif
{
    Vehicle *ambulance = (Vehicle *)arg;
    CityMap *city_map = (CityMap *)ambulance->city_map;

    vehicle_set_render_symbol(ambulance, 'A');

    if (!ambulance_place_initially(ambulance))
        return 0;

    int last_tick = global_clock_current_tick();

    while (simulation_running && ambulance->active)
    {
        // Aguarda a sincronização do relógio global (sem busy-wait)
        wait_next_tick(last_tick);
        last_tick = global_clock_current_tick();

        if (!simulation_running || !ambulance->active)
        {
            break;
        }

        // A ambulância move-se a cada tick (velocidade = 1)
        Road *road = ambulance->current_road;
        int index = ambulance->road_cell_index;
        Intersection *current_intersection = road_get_intersection(road, index);

        // Determina a próxima célula
        int next_index = index + 1;
        if (next_index >= road->cell_count)
        {
            next_index = 0; // Wrap around
        }

        Cell *next_cell = road_get_cell(road, next_index);
        if (next_cell == NULL)
        {
            continue;
        }

        // Verifica se a próxima célula coincide com um cruzamento
        bool next_is_intersection = road_has_intersection(road, next_index);
        Intersection *intersection = NULL;

        if (next_is_intersection)
        {
            intersection = city_map_get_intersection(city_map, next_cell->row, next_cell->column);
            if (intersection != NULL)
            {
                int priority_changed =
                    intersection_request_ambulance_priority(intersection, road->direction);

                if (priority_changed)
                {
                    wait_next_tick(last_tick);
                    last_tick = global_clock_current_tick();

                    if (!simulation_running || !ambulance->active)
                    {
                        intersection_clear_ambulance_priority(intersection);
                        break;
                    }
                }

                pthread_mutex_lock(&intersection->mutex);
                intersection_wait_green(intersection, road->direction);
                pthread_mutex_unlock(&intersection->mutex);
            }
        }

        // Move usando a mesma estrutura base de Vehicle.
        if (vehicle_try_move_to_road_index(ambulance, next_index))
        {
            if (current_intersection != NULL)
                intersection_clear_ambulance_priority(current_intersection);

            // Se a nova célula ocupada é um cruzamento, decide se vai virar
            if (next_is_intersection && intersection != NULL)
            {
                // Decisão de curva com probabilidade (ex: 30%)
                if ((rand() % 100) < 30)
                {
                    Road *new_road = (road->direction == ROAD_HORIZONTAL)
                                         ? intersection->vertical_road
                                         : intersection->horizontal_road;
                    if (new_road != NULL)
                    {
                        int new_index = (new_road->direction == ROAD_HORIZONTAL)
                                            ? next_cell->column
                                            : next_cell->row;
                        if (new_index >= 0 && new_index < new_road->cell_count)
                        {
                            ambulance->current_road = new_road;
                            ambulance->road_cell_index = new_index;
                        }
                    }
                }
            }
        }
    }

    // Libera a célula atual ao encerrar
    Intersection *current_intersection =
        road_get_intersection(ambulance->current_road, ambulance->road_cell_index);
    Cell *current_cell = road_get_cell(ambulance->current_road, ambulance->road_cell_index);
    if (current_intersection != NULL)
    {
        intersection_clear_ambulance_priority(current_intersection);
    }

    if (current_cell != NULL)
    {
        cell_release(current_cell);
    }

    return 0;
}
