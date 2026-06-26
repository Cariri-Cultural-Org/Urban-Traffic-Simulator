#include <time.h>

#include "GlobalClock.h"

int global_tick = 0;
pthread_mutex_t clock_mutex;
pthread_cond_t clock_cond;
bool simulation_running = true;

static void sleep_one_tick(void)
{
    const struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 100000000
    };

    nanosleep(&delay, NULL);
}

void clock_init(void)
{
    global_tick = 0;
    simulation_running = true;
    pthread_mutex_init(&clock_mutex, NULL);
    pthread_cond_init(&clock_cond, NULL);
}

void *clock_thread(void *arg)
{
    (void)arg;

    while (1)
    {
        sleep_one_tick();

        pthread_mutex_lock(&clock_mutex);

        if (!simulation_running)
        {
            pthread_cond_broadcast(&clock_cond);
            pthread_mutex_unlock(&clock_mutex);
            break;
        }

        global_tick++;
        pthread_cond_broadcast(&clock_cond);
        pthread_mutex_unlock(&clock_mutex);
    }

    return NULL;
}

void wait_next_tick(int current_tick)
{
    pthread_mutex_lock(&clock_mutex);

    while (global_tick == current_tick && simulation_running)
        pthread_cond_wait(&clock_cond, &clock_mutex);

    pthread_mutex_unlock(&clock_mutex);
}

int clock_get_tick(void)
{
    pthread_mutex_lock(&clock_mutex);
    int tick = global_tick;
    pthread_mutex_unlock(&clock_mutex);

    return tick;
}

int simulation_is_running(void)
{
    pthread_mutex_lock(&clock_mutex);
    int running = simulation_running;
    pthread_mutex_unlock(&clock_mutex);

    return running;
}

void simulation_stop(void)
{
    pthread_mutex_lock(&clock_mutex);
    simulation_running = false;
    pthread_cond_broadcast(&clock_cond);
    pthread_mutex_unlock(&clock_mutex);
}

void clock_destroy(void)
{
    pthread_cond_destroy(&clock_cond);
    pthread_mutex_destroy(&clock_mutex);
}
