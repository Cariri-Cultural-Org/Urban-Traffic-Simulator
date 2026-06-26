#include "TrafficLight.h"

void traffic_light_init(TrafficLight *light, int id, int green_ticks, int red_ticks)
{
    light->id = id;
    light->state = LIGHT_RED;
    light->green_ticks = green_ticks;
    light->red_ticks = red_ticks;
    light->elapsed_ticks = 0;
    pthread_mutex_init(&light->mutex, NULL);
}

void traffic_light_update(TrafficLight *light)
{
    pthread_mutex_lock(&light->mutex);

    light->elapsed_ticks++;

    if (light->state == LIGHT_RED && light->elapsed_ticks >= light->red_ticks)
    {
        light->state = LIGHT_GREEN;
        light->elapsed_ticks = 0;
    }
    else if (light->state == LIGHT_GREEN && light->elapsed_ticks >= light->green_ticks)
    {
        light->state = LIGHT_RED;
        light->elapsed_ticks = 0;
    }

    pthread_mutex_unlock(&light->mutex);
}

void traffic_light_destroy(TrafficLight *light)
{
    pthread_mutex_destroy(&light->mutex);
}
