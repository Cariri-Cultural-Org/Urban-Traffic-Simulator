#include <stdio.h>
#include <string.h>

#include "models/CityMap.h"
#include "models/CityMapRenderer.h"
#include "models/Intersection.h"
#include "models/Cell.h"

static int failures = 0;

#define ASSERT_TRUE(condition)                                                       \
    do                                                                              \
    {                                                                               \
        if (!(condition))                                                            \
        {                                                                           \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                             \
        }                                                                           \
    } while (0)

static int render_to_buffer(CityMap *city_map, char *buffer, size_t buffer_size)
{
    FILE *stream = tmpfile();
    size_t bytes_read;

    if (!stream)
        return 0;

    if (!city_map_render_ascii(city_map, stream, 7))
    {
        fclose(stream);
        return 0;
    }

    rewind(stream);
    bytes_read = fread(buffer, sizeof(char), buffer_size - 1, stream);
    buffer[bytes_read] = '\0';
    fclose(stream);
    return 1;
}

static int line_length_before_newline(const char *line)
{
    const char *newline = strchr(line, '\n');

    if (!newline)
        return -1;

    return (int)(newline - line);
}

static void mark_cell(Cell *cell, char symbol)
{
    pthread_mutex_lock(&cell->mutex);
    cell->occupied = 1;
    cell->occupant_symbol = symbol;
    cell->vehicle = NULL;
    pthread_mutex_unlock(&cell->mutex);
}

static void test_renderer_frame_shape_and_symbols(void)
{
    CityMap *city_map = city_map_create();
    char output[4096];
    char *line;
    int map_lines = 0;

    ASSERT_TRUE(city_map != NULL);
    if (!city_map)
        return;

    mark_cell(city_map_get_cell(city_map, 4, 0), 'C');
    mark_cell(city_map_get_cell(city_map, 0, 8), 'A');
    intersection_toggle_signal(city_map_get_intersection(city_map, 4, 8));

    ASSERT_TRUE(render_to_buffer(city_map, output, sizeof(output)) == 1);
    ASSERT_TRUE(strstr(output, "Tick 7") != NULL);
    ASSERT_TRUE(strchr(output, '-') != NULL);
    ASSERT_TRUE(strchr(output, '|') != NULL);
    ASSERT_TRUE(strchr(output, 'C') != NULL);
    ASSERT_TRUE(strchr(output, 'A') != NULL);
    ASSERT_TRUE(strchr(output, 'H') != NULL);
    ASSERT_TRUE(strchr(output, 'V') != NULL);

    line = strchr(output, '\n');
    ASSERT_TRUE(line != NULL);
    if (line)
        line++;

    while (line && *line && map_lines < CITY_MAP_ROWS)
    {
        ASSERT_TRUE(line_length_before_newline(line) == CITY_MAP_COLUMNS);
        line = strchr(line, '\n');
        if (line)
            line++;
        map_lines++;
    }

    ASSERT_TRUE(map_lines == CITY_MAP_ROWS);
    city_map_destroy(city_map);
}

int main(void)
{
    test_renderer_frame_shape_and_symbols();

    if (failures)
    {
        fprintf(stderr, "%d renderer test(s) failed.\n", failures);
        return 1;
    }

    puts("test_city_map_renderer: ok");
    return 0;
}
