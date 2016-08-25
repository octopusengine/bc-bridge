#ifndef _BC_OS_H
#define _BC_OS_H

#include <bc/common.h>
#include <bc/tick.h>

#include <pthread.h>

typedef pthread_mutex_t bc_os_mutex_t;

void bc_os_sleep(bc_tick_t timeout);

#endif /* _BC_OS_H */
