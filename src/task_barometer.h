#ifndef _TASK_BAROMETER_H
#define _TASK_BAROMETER_H

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

} task_barometer_t;

void task_barometer_spawn(bc_bridge_t *bridge, task_info_t *task_info);

void task_barometer_set_interval(task_barometer_t *self, bc_tick_t interval);
void task_barometer_get_interval(task_barometer_t *self, bc_tick_t *interval);

#endif /* _TASK_BAROMETER_H */
