#ifndef _TASK_HUMIDITY_H
#define _TASK_HUMIDITY_H

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
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

    bool _quit;

} task_humidity_t;

void task_humidity_spawn(bc_bridge_t *bridge, task_info_t *task_info);
void task_humidity_terminate(task_humidity_t *self);
bool task_humidity_is_quit_request(task_humidity_t *self);
void task_humidity_set_interval(task_humidity_t *self, bc_tick_t interval);
void task_humidity_get_interval(task_humidity_t *self, bc_tick_t *interval);

#endif /* _TASK_HUMIDITY_H */
