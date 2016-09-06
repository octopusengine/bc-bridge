#include "task_relay.h"
#include "bc_log.h"
#include "bc_talk.h"
#include "bc_tag.h"
#include "bc_bridge.h"

static void *task_relay_worker(void *parameter);

task_relay_t *task_relay_spawn(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address)
{
    task_relay_t *self = (task_relay_t *) malloc(sizeof(task_relay_t));

    self->_bridge = bridge;
    self->_i2c_channel = i2c_channel;
    self->_device_address = device_address;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_relay_worker, self);

    return self;
}


void task_relay_set_mode(task_relay_t *self, bc_module_relay_mode_t relay_mode)
{
    bc_os_mutex_lock(&self->mutex);
    self->_relay_mode = relay_mode;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_relay_worker(void *parameter)
{

    task_relay_t *self = (task_relay_t *) parameter;

    bc_module_relay_mode_t relay_mode;

    bc_log_info("task_relay_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_tag_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_module_relay_t module_relay;

    bc_module_relay_init(&module_relay, &interface, self->_device_address);

    while (true)
    {

        bc_os_semaphore_get(&self->semaphore);
        bc_log_debug("task_relay_worker: wake up signal");

        bc_os_mutex_lock(&self->mutex);
        relay_mode = self->_relay_mode;
        bc_os_mutex_unlock(&self->mutex);

        bc_module_relay_set_mode(&module_relay, relay_mode);
    }

    return NULL;
}
