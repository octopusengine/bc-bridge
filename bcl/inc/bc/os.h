#ifndef _BC_OS_H
#define _BC_OS_H

#include <bc/tick.h>
#include <pthread.h>

typedef pthread_mutex_t bc_os_mutex_t;

void bc_os_mutex_init(bc_os_mutex_t *mutex);

void bc_os_mutex_lock(bc_os_mutex_t *mutex);

void bc_os_mutex_unlock(bc_os_mutex_t *mutex);

void bc_os_sleep(bc_tick_t milisec);

#endif /* _BC_OS_H */
