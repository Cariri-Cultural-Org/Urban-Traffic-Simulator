#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <pthread.h>

typedef enum
{
    LIGHT_RED,
    LIGHT_GREEN
} TrafficLightState;

typedef struct
{
    int id;
    TrafficLightState state;
    int green_ticks;
    int red_ticks;
    int elapsed_ticks;
    pthread_mutex_t mutex;
} TrafficLight;

void traffic_light_init(TrafficLight *light, int id, int green_ticks, int red_ticks);
void traffic_light_update(TrafficLight *light);
void traffic_light_destroy(TrafficLight *light);

#endif
