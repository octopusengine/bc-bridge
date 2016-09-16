#ifndef _TASK_THERMOMETER_H
#define _TASK_THERMOMETER_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

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

    bc_i2c_interface_t _i2c_interface;
    uint8_t _device_address;

    bool _quit;

} task_thermometer_t;

void task_thermometer_spawn(bc_bridge_t *bridge, task_info_t *task_info);
void task_thermometer_terminate(task_thermometer_t *self);
void task_thermometer_set_interval(task_thermometer_t *self, bc_tick_t interval);
void task_thermometer_get_interval(task_thermometer_t *self, bc_tick_t *interval);
bool task_thermometer_is_quit_request(task_thermometer_t *self);

#endif /* _TASK_THERMOMETER_H */
