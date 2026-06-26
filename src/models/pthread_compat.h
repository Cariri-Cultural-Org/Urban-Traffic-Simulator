#ifndef PTHREAD_COMPAT_H
#define PTHREAD_COMPAT_H

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>

typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef HANDLE pthread_t;

typedef void *(*pthread_start_routine_t)(void *);

typedef struct
{
    pthread_start_routine_t start_routine;
    void *argument;
} pthread_thread_wrapper_t;

static DWORD WINAPI pthread_thread_trampoline(LPVOID param)
{
    pthread_thread_wrapper_t *wrapper = (pthread_thread_wrapper_t *)param;
    void *result = wrapper->start_routine(wrapper->argument);
    free(wrapper);
    (void)result;
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const void *attr)
{
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    EnterCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
    return 0;
}

static inline int pthread_cond_init(pthread_cond_t *condition, const void *attr)
{
    (void)attr;
    InitializeConditionVariable(condition);
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *condition)
{
    (void)condition;
    return 0;
}

static inline int pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
    return SleepConditionVariableCS(condition, mutex, INFINITE) ? 0 : 1;
}

static inline int pthread_cond_broadcast(pthread_cond_t *condition)
{
    WakeAllConditionVariable(condition);
    return 0;
}

static inline int pthread_create(
    pthread_t *thread,
    const void *attr,
    pthread_start_routine_t start_routine,
    void *argument)
{
    pthread_thread_wrapper_t *wrapper = (pthread_thread_wrapper_t *)malloc(sizeof(*wrapper));

    if (!wrapper)
        return 1;

    (void)attr;
    wrapper->start_routine = start_routine;
    wrapper->argument = argument;

    *thread = CreateThread(NULL, 0, pthread_thread_trampoline, wrapper, 0, NULL);

    if (!*thread)
    {
        free(wrapper);
        return 1;
    }

    return 0;
}

static inline int pthread_join(pthread_t thread, void **result)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    if (result)
        *result = NULL;

    return 0;
}

#else
#include <pthread.h>
#endif

#endif