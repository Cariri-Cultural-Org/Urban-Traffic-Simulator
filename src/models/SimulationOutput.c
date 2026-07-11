#include "SimulationOutput.h"

#include <stdarg.h>

#include "models/CityMapRenderer.h"
#include "models/pthread_compat.h"

static pthread_mutex_t output_mutex;
static int output_initialized = 0;
static int log_enabled = 1;
static FILE *log_stream = NULL;
static int owns_log_stream = 0;

void simulation_output_init(void)
{
    if (output_initialized)
        return;

    pthread_mutex_init(&output_mutex, NULL);
    log_stream = stderr;
    owns_log_stream = 0;
    output_initialized = 1;
}

void simulation_output_destroy(void)
{
    if (!output_initialized)
        return;

    pthread_mutex_lock(&output_mutex);
    if (owns_log_stream && log_stream)
        fclose(log_stream);
    log_stream = NULL;
    owns_log_stream = 0;
    pthread_mutex_unlock(&output_mutex);

    pthread_mutex_destroy(&output_mutex);
    output_initialized = 0;
}

void simulation_output_set_log_enabled(int enabled)
{
    log_enabled = enabled;
}

int simulation_output_set_log_file(const char *path)
{
    FILE *stream;

    if (!path)
        return 0;

    stream = fopen(path, "w");
    if (!stream)
        return 0;

    if (output_initialized)
        pthread_mutex_lock(&output_mutex);

    if (owns_log_stream && log_stream)
        fclose(log_stream);

    log_stream = stream;
    owns_log_stream = 1;
    log_enabled = 1;

    if (output_initialized)
        pthread_mutex_unlock(&output_mutex);

    return 1;
}

void simulation_output_log(const char *format, ...)
{
    va_list args;

    if (!log_enabled)
        return;

    if (output_initialized)
        pthread_mutex_lock(&output_mutex);

    va_start(args, format);
    vfprintf(log_stream ? log_stream : stderr, format, args);
    va_end(args);
    fflush(log_stream ? log_stream : stderr);

    if (output_initialized)
        pthread_mutex_unlock(&output_mutex);
}

int simulation_output_render_city_map(CityMap *city_map, FILE *stream, int tick)
{
    CityMapAsciiRenderOptions options = CITY_MAP_ASCII_RENDER_DEFAULT_OPTIONS;
    int result;

    options.show_tick = 1;

    if (output_initialized)
        pthread_mutex_lock(&output_mutex);

    result = city_map_render_ascii_with_options(city_map, stream, tick, &options);

    if (output_initialized)
        pthread_mutex_unlock(&output_mutex);

    return result;
}
