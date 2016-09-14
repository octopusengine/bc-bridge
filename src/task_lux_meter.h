#ifndef _TASK_LUX_METER_H
#define _TASK_LUX_METER_H

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

} task_lux_meter_t;

void task_lux_meter_spawn(bc_bridge_t *bridge, task_info_t *task_info);
void task_lux_meter_terminate(task_lux_meter_t *self);

void task_lux_meter_set_interval(task_lux_meter_t *self, bc_tick_t interval);
void task_lux_meter_get_interval(task_lux_meter_t *self, bc_tick_t *interval);

#endif /* _TASK_LUX_METER_H */
