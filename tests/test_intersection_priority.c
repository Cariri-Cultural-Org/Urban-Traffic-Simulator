#include <stdio.h>

#include "models/CityMap.h"
#include "models/Intersection.h"
#include "models/city_map_utils.h"

static int failures = 0;

typedef struct
{
    Intersection *intersection;
    RoadDirection direction;
    int result;
} PriorityRequest;

#define ASSERT_TRUE(condition)                                                   \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                        \
        {                                                                       \
            fprintf(stderr, "%s:%d: falhou: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                         \
        }                                                                       \
    } while (0)

static void *request_priority(void *arg)
{
    PriorityRequest *request = (PriorityRequest *)arg;

    request->result = intersection_request_ambulance_priority(
        request->intersection,
        request->direction
    );
    return NULL;
}

static void test_priority_clear_restores_previous_signal_and_cycle(void)
{
    Intersection *intersection = intersection_create(1, 4, 8, NULL, NULL);

    ASSERT_TRUE(intersection != NULL);
    if (!intersection)
        return;

    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);

    ASSERT_TRUE(intersection_request_ambulance_priority(intersection, ROAD_VERTICAL) == 1);
    ASSERT_TRUE(intersection->ambulance_present == 1);
    ASSERT_TRUE(intersection->ambulance_direction == ROAD_VERTICAL);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);
    ASSERT_TRUE(intersection->previous_green_direction == ROAD_HORIZONTAL);

    intersection_clear_ambulance_priority(intersection);

    ASSERT_TRUE(intersection->ambulance_present == 0);
    ASSERT_TRUE(intersection->ambulance_direction == ROAD_HORIZONTAL);
    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);

    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_destroy(intersection);
}

static void test_priority_clear_keeps_cycle_when_signal_was_already_open(void)
{
    Intersection *intersection = intersection_create(2, 4, 16, NULL, NULL);

    ASSERT_TRUE(intersection != NULL);
    if (!intersection)
        return;

    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    ASSERT_TRUE(intersection_request_ambulance_priority(intersection, ROAD_VERTICAL) == 0);
    ASSERT_TRUE(intersection->ambulance_present == 1);
    ASSERT_TRUE(intersection->previous_green_direction == ROAD_VERTICAL);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_clear_ambulance_priority(intersection);
    ASSERT_TRUE(intersection->ambulance_present == 0);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);

    intersection_destroy(intersection);
}

static void test_priority_holds_signal_until_clear(void)
{
    Intersection *intersection = intersection_create(3, 10, 24, NULL, NULL);

    ASSERT_TRUE(intersection != NULL);
    if (!intersection)
        return;

    ASSERT_TRUE(intersection_request_ambulance_priority(intersection, ROAD_VERTICAL) == 1);

    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->ambulance_present == 1);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_clear_ambulance_priority(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);

    intersection_destroy(intersection);
}

static void test_signal_toggle_is_deferred_during_crossing(void)
{
    Intersection *intersection = intersection_create(4, 10, 8, NULL, NULL);

    ASSERT_TRUE(intersection != NULL);
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);
    intersection->crossing_occupied = 1;
    intersection->crossing_direction = ROAD_HORIZONTAL;
    pthread_mutex_unlock(&intersection->mutex);

    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);

    intersection_finish_crossing(intersection, 0);
    intersection_toggle_signal(intersection);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_destroy(intersection);
}

static void test_conflicting_priority_waits_for_crossing_to_finish(void)
{
    Intersection *intersection = intersection_create(5, 10, 16, NULL, NULL);
    PriorityRequest request = {intersection, ROAD_VERTICAL, 0};
    pthread_t request_thread;
    int create_result;

    ASSERT_TRUE(intersection != NULL);
    if (!intersection)
        return;

    pthread_mutex_lock(&intersection->mutex);
    intersection->crossing_occupied = 1;
    intersection->crossing_direction = ROAD_HORIZONTAL;
    pthread_mutex_unlock(&intersection->mutex);

    create_result = pthread_create(&request_thread, NULL, request_priority, &request);
    ASSERT_TRUE(create_result == 0);
    if (create_result != 0)
    {
        intersection_finish_crossing(intersection, 0);
        intersection_destroy(intersection);
        return;
    }

    thread_sleep_ms(50);

    pthread_mutex_lock(&intersection->mutex);
    ASSERT_TRUE(intersection->green_direction == ROAD_HORIZONTAL);
    ASSERT_TRUE(intersection->ambulance_present == 0);
    pthread_mutex_unlock(&intersection->mutex);

    intersection_finish_crossing(intersection, 0);
    pthread_join(request_thread, NULL);

    ASSERT_TRUE(request.result == 1);
    ASSERT_TRUE(intersection->ambulance_present == 1);
    ASSERT_TRUE(intersection->green_direction == ROAD_VERTICAL);

    intersection_clear_ambulance_priority(intersection);
    intersection_destroy(intersection);
}

static void test_city_map_intersections_can_be_released_before_destroy(void)
{
    CityMap *city_map = city_map_create();

    ASSERT_TRUE(city_map != NULL);
    if (!city_map)
        return;

    ASSERT_TRUE(city_map->intersection_count == 12);
    ASSERT_TRUE(city_map_get_intersection(city_map, 4, 8) != NULL);

    release_intersections(city_map);
    ASSERT_TRUE(city_map->intersections == NULL);

    city_map_destroy(city_map);
}

static void test_city_map_create_destroy_repeatedly(void)
{
    for (int attempt = 0; attempt < 5; attempt++)
    {
        CityMap *city_map = city_map_create();

        ASSERT_TRUE(city_map != NULL);
        if (!city_map)
            return;

        ASSERT_TRUE(city_map->intersection_count == 12);
        ASSERT_TRUE(city_map_get_intersection(city_map, 16, 32) != NULL);

        city_map_destroy(city_map);
    }
}

int main(void)
{
    test_priority_clear_restores_previous_signal_and_cycle();
    test_priority_clear_keeps_cycle_when_signal_was_already_open();
    test_priority_holds_signal_until_clear();
    test_signal_toggle_is_deferred_during_crossing();
    test_conflicting_priority_waits_for_crossing_to_finish();
    test_city_map_intersections_can_be_released_before_destroy();
    test_city_map_create_destroy_repeatedly();

    if (failures)
    {
        fprintf(stderr, "%d teste(s) falharam.\n", failures);
        return 1;
    }

    puts("Todos os testes A4-D passaram.");
    return 0;
}
