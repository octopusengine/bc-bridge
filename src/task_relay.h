#ifndef _TASK_RELAY_H
#define _TASK_RELAY_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "bc_module_relay.h"
#include "task.h"


typedef enum
{
    TASK_RELAY_MODE_FALSE = BC_MODULE_RELAY_STATE_T, //red led
    TASK_RELAY_MODE_TRUE = BC_MODULE_RELAY_STATE_F,  //green led
    TASK_RELAY_MODE_NULL = -1

} task_relay_mode_t;


typedef struct
{
    bc_os_task_t task;
    bc_os_mutex_t mutex;
    bc_os_semaphore_t semaphore;

    task_relay_mode_t _relay_mode;
    bc_tick_t _tick_last_feed;

    bc_bridge_t *_bridge;
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

    bool _quit;

} task_relay_t;

void task_relay_spawn(bc_bridge_t *bridge, task_info_t *task_info);
void task_relay_terminate(task_relay_t *self);
void task_relay_set_mode(task_relay_t *self, task_relay_mode_t relay_mode);
void task_relay_get_mode(task_relay_t *self, task_relay_mode_t *relay_mode);
bool task_relay_is_quit_request(task_relay_t *self);

#endif /* _TASK_RELAY_H */
