#ifndef _TASK_RELAY_H
#define _TASK_RELAY_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "bc_module_relay.h"

typedef struct
{
    bc_os_task_t task;
    bc_os_mutex_t mutex;
    bc_os_semaphore_t semaphore;

    bc_module_relay_mode_t _relay_mode;

    bc_bridge_t *_bridge;
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

} task_relay_t;

task_relay_t *task_relay_spawn(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address);
void task_relay_set_mode(task_relay_t *self, bc_module_relay_mode_t relay_mode);

#endif /* _TASK_RELAY_H */
