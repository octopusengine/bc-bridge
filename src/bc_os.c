#include "bc_os.h"
#include "bc_log.h"
#include <pthread.h>
#include <semaphore.h>

// TODO Budeme resit kdyz se nepovede malloc ? To uz je stejne konec sveta...

void bc_os_task_init(bc_os_task_t *task, void *(*task_function)(void *), void *parameter)
{
    task->_task = malloc(sizeof(pthread_t));

    if (pthread_create((pthread_t *) task->_task, NULL, task_function, parameter) != 0)
    {
        bc_log_fatal("bc_os_task_init: call failed: pthread_create");
    }
}

void bc_os_task_destroy(bc_os_task_t *task)
{
    if (pthread_join(*((pthread_t *) task->_task), NULL) != 0)
    {
        bc_log_fatal("bc_os_task_destroy: call failed: pthread_join");
    }

    free(task->_task);
}

uint64_t bc_os_task_get_id(void)
{
    return (uint64_t) pthread_self();
}

void bc_os_task_sleep(bc_tick_t timeout)
{
    if (usleep(timeout * 1000UL) != 0)
    {
        bc_log_fatal("bc_os_sleep: call failed: usleep");
    }
}

void bc_os_mutex_init(bc_os_mutex_t *mutex)
{
    mutex->_mutex = malloc(sizeof(pthread_mutex_t));

    if (pthread_mutex_init((pthread_mutex_t *) mutex->_mutex, NULL) != 0)
    {
        bc_log_fatal("bc_os_mutex_init: call failed: pthread_mutex_init");
    }
}

void bc_os_mutex_destroy(bc_os_mutex_t *mutex)
{
    if (pthread_mutex_destroy((pthread_mutex_t *) mutex->_mutex) != 0)
    {
        bc_log_fatal("bc_os_mutex_destroy: call failed: pthread_mutex_destroy");
    }

    free(mutex->_mutex);
}

void bc_os_mutex_lock(bc_os_mutex_t *mutex)
{
    if (pthread_mutex_lock((pthread_mutex_t *) mutex->_mutex) != 0)
    {
        bc_log_fatal("bc_os_mutex_lock: call failed: pthread_mutex_lock");
    }
}

void bc_os_mutex_unlock(bc_os_mutex_t *mutex)
{
    if (pthread_mutex_unlock((pthread_mutex_t *) mutex->_mutex) != 0)
    {
        bc_log_fatal("bc_os_mutex_unlock: call failed: pthread_mutex_unlock");
    }
}

void bc_os_semaphore_init(bc_os_semaphore_t *semaphore, uint32_t value)
{
    semaphore->_semaphore = malloc(sizeof(sem_t));

    if (sem_init((sem_t *) semaphore->_semaphore, 0, value) != 0)
    {
        bc_log_fatal("bc_os_semaphore_init: call failed: sem_init");
    }
}

void bc_os_semaphore_destroy(bc_os_semaphore_t *semaphore)
{
    if (sem_destroy((sem_t *) semaphore->_semaphore) != 0)
    {
        bc_log_fatal("bc_os_semaphore_destroy: call failed: sem_destroy");
    }

    free(semaphore->_semaphore);
}

void bc_os_semaphore_put(bc_os_semaphore_t *semaphore)
{
    if (sem_post((sem_t *) semaphore->_semaphore) != 0)
    {
        bc_log_fatal("bc_os_semaphore_put: call failed: sem_post");
    }
}

void bc_os_semaphore_get(bc_os_semaphore_t *semaphore)
{
    if (sem_wait((sem_t *) semaphore->_semaphore) != 0)
    {
        bc_log_fatal("bc_os_semaphore_get: call failed: sem_wait");
    }
}

bool bc_os_semaphore_timed_get(bc_os_semaphore_t *semaphore, bc_tick_t timeout)
{
    if (timeout == 0)
    {
        if (sem_trywait((sem_t *) semaphore->_semaphore) == 0)
        {
            return true;
        }

        if (errno != EAGAIN)
        {
            bc_log_fatal("bc_os_semaphore_timed_get: call failed: sem_trywait");
        }
    }
    else
    {
        struct timespec ts;

        // TODO Problem je, ze Linux pravdepodobne nepodporuje MONOTONIC clock na semaphore
        if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        {
            bc_log_fatal("bc_os_semaphore_timed_get: call failed: clock_gettime");
        }

        ts.tv_sec += timeout / 1000UL;
        ts.tv_nsec += (timeout % 1000UL) * 1000000UL;
        ts.tv_sec += ts.tv_nsec / 1000000000UL;
        ts.tv_nsec %= 1000000000UL;

        if (sem_timedwait((sem_t *) semaphore->_semaphore, &ts) == 0)
        {
            return true;
        }

        if (errno != ETIMEDOUT)
        {
            bc_log_fatal("bc_os_semaphore_timed_get: call failed: sem_timedwait");
        }
    }

    return false;
}
