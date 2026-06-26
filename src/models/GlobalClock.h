#ifndef GLOBAL_CLOCK_H
#define GLOBAL_CLOCK_H

#include <pthread.h>
#include <stdbool.h>

extern int global_tick;
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern bool simulation_running;

void clock_init(void);
void *clock_thread(void *arg);
void wait_next_tick(int current_tick);
int clock_get_tick(void);
int simulation_is_running(void);
void simulation_stop(void);
void clock_destroy(void);

#endif
