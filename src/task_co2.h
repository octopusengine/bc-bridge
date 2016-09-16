#ifndef _TASK_C02_H
#define _TASK_C02_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "task.h"

typedef struct
{
    bc_os_task_t task;
    bc_os_mutex_t mutex;
    bc_os_semaphore_t semaphore;

    bc_tick_t tick_feed_interval;
    bc_tick_t _tick_last_feed;

    bc_bridge_t *_bridge;

    bool _quit;

} task_co2_t;

void task_co2_spawn(bc_bridge_t *bridge, task_info_t *task_info);
void task_co2_terminate(task_co2_t *self);
bool task_co2_is_quit_request(task_co2_t *self);
void task_co2_set_interval(task_co2_t *self, bc_tick_t interval);
void task_co2_get_interval(task_co2_t *self, bc_tick_t *interval);

#endif /* _TASK_C02_H */
