#include "task_relay.h"
#include "bc_log.h"
#include "bc_talk.h"

void task_relay_set_state(task_info_t *task_info, task_relay_state_t state)
{
    task_lock(task_info);
    ((task_relay_parameters_t *)task_info->parameters)->state = state;
    task_unlock(task_info);

    task_semaphore_put(task_info);
}

void task_relay_get_state(task_info_t *task_info, task_relay_state_t *state)
{
    task_lock(task_info);
    *state = ((task_relay_parameters_t *)task_info->parameters)->state;
    task_unlock(task_info);
}

void *task_relay_worker(void *task_parameter)
{

    task_worker_t *self = (task_worker_t *) task_parameter;
    task_relay_parameters_t *parameters = (task_relay_parameters_t *)self->parameters;

    task_relay_state_t relay_state;

    bc_log_info("task_relay_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_module_relay_t module_relay;

    if (!bc_module_relay_init(&module_relay, &interface, self->_device_address))
    {
        bc_log_debug("task_relay_worker: bc_module_relay_init false");

        return NULL;
    }

    task_worker_set_init_done(self);

    while (true)
    {

        bc_os_semaphore_get(&self->semaphore);

        bc_log_debug("task_relay_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_relay_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        bc_os_mutex_lock(self->mutex);
        relay_state = parameters->state;
        bc_os_mutex_unlock(self->mutex);

        if (relay_state != TASK_RELAY_STATE_NULL)
        {

            if (!bc_module_relay_set_state(&module_relay, relay_state == TASK_RELAY_STATE_FALSE ? BC_MODULE_RELAY_STATE_TRUE
                                                                                              : BC_MODULE_RELAY_STATE_FALSE))
            {
                bc_log_error("task_relay_worker: bc_module_relay_set_state");
                break;
            }

            bc_talk_publish_relay((int) relay_state, self->_device_address);
            bc_os_task_sleep( 100L - (bc_tick_get() - self->_tick_last_feed) ); // click one per 100 ms
        }

    }

    return NULL;
}
