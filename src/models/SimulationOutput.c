#include "SimulationOutput.h"

#include <stdarg.h>

#include "models/CityMapRenderer.h"
#include "models/pthread_compat.h"

static pthread_mutex_t output_mutex;
static int output_initialized = 0;

void simulation_output_init(void)
{
    if (output_initialized)
        return;

    pthread_mutex_init(&output_mutex, NULL);
    output_initialized = 1;
}

void simulation_output_destroy(void)
{
    if (!output_initialized)
        return;

    pthread_mutex_destroy(&output_mutex);
    output_initialized = 0;
}

void simulation_output_log(const char *format, ...)
{
    va_list args;

    if (output_initialized)
        pthread_mutex_lock(&output_mutex);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);

    if (output_initialized)
        pthread_mutex_unlock(&output_mutex);
}

int simulation_output_render_city_map(const CityMap *city_map, FILE *stream, int tick)
{
    int result;

    if (output_initialized)
        pthread_mutex_lock(&output_mutex);

    result = city_map_render_ascii(city_map, stream, tick);

    if (output_initialized)
        pthread_mutex_unlock(&output_mutex);

    return result;
}
