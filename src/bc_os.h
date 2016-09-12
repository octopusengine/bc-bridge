#ifndef _BC_OS_H
#define _BC_OS_H

#include "bc_common.h"
#include "bc_tick.h"

typedef struct
{
    void *_task;

} bc_os_task_t;

typedef struct
{
    void *_mutex;

} bc_os_mutex_t;

typedef struct
{
    void *_semaphore;

} bc_os_semaphore_t;

void bc_os_task_init(bc_os_task_t *task, void *(*task_function)(void *), void *parameter);
void bc_os_task_destroy(bc_os_task_t *task);
void bc_os_task_sleep(bc_tick_t timeout);
void bc_os_mutex_init(bc_os_mutex_t *mutex);
void bc_os_mutex_destroy(bc_os_mutex_t *mutex);
void bc_os_mutex_lock(bc_os_mutex_t *mutex);
void bc_os_mutex_unlock(bc_os_mutex_t *mutex);
void bc_os_semaphore_init(bc_os_semaphore_t *semaphore, uint32_t value);
void bc_os_semaphore_destroy(bc_os_semaphore_t *semaphore);
void bc_os_semaphore_put(bc_os_semaphore_t *semaphore);
void bc_os_semaphore_get(bc_os_semaphore_t *semaphore);
bool bc_os_semaphore_timed_get(bc_os_semaphore_t *semaphore, bc_tick_t timeout);

#endif /* _BC_OS_H */
