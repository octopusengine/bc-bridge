#ifndef _TASK_THERMOMETER_H
#define _TASK_THERMOMETER_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"

typedef struct
{
    bc_bridge_t *bridge;
    bc_bridge_i2c_channel_t i2c_channel;
    uint8_t device_address;

} task_thermometer_parameters_t;

typedef struct
{
    bc_os_task_t task;
    bc_os_mutex_t mutex;
    bc_os_semaphore_t semaphore;

    bc_tick_t _tick_feed_interval;
    bc_tick_t _tick_last_feed;

    bc_bridge_t *_bridge;
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

} task_thermometer_t;

task_thermometer_t *task_thermometer_spawn(task_thermometer_parameters_t *parameters);

void task_thermometer_set_interval(task_thermometer_t *self, bc_tick_t interval);

#endif /* _TASK_THERMOMETER_H */
