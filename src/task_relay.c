#include "task_relay.h"
#include "bc_log.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_relay_worker(void *parameter);

void task_relay_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_relay_t *self = (task_relay_t *) malloc(sizeof(task_relay_t));

    if (self == NULL)
    {
        bc_log_fatal("task_relay_spawn: call failed: malloc");
    }

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_relay_worker, self);

    self->_relay_mode = TASK_RELAY_MODE_NULL;

    task_info->task = self;
    task_info->enabled = true;
}


void task_relay_set_mode(task_relay_t *self, task_relay_mode_t relay_mode)
{
    bc_os_mutex_lock(&self->mutex);
    self->_relay_mode = relay_mode;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_relay_get_mode(task_relay_t *self, task_relay_mode_t *relay_mode)
{
    bc_os_mutex_lock(&self->mutex);
    *relay_mode =  self->_relay_mode;
    bc_os_mutex_unlock(&self->mutex);

}

static void *task_relay_worker(void *parameter)
{

    task_relay_t *self = (task_relay_t *) parameter;

    bool init_ok = false;
    task_relay_mode_t last_mode = TASK_RELAY_MODE_NULL;
    task_relay_mode_t relay_mode;

    bc_log_info("task_relay_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_module_relay_t module_relay;


    while (true)
    {

        if (init_ok==false) //TODO predelat do task manageru
        {
            if (!bc_module_relay_init(&module_relay, &interface, self->_device_address))
            {
                bc_log_error("task_relay_worker: bc_module_relay_init");
                bc_os_task_sleep(1000);
                continue;
            }
            init_ok = true;
        }

        bc_os_semaphore_get(&self->semaphore);
        self->_tick_last_feed = bc_tick_get();

        bc_log_debug("task_relay_worker: wake up signal");

        bc_os_mutex_lock(&self->mutex);
        relay_mode = self->_relay_mode;
        bc_os_mutex_unlock(&self->mutex);

        if (relay_mode!=TASK_RELAY_MODE_NULL){

            if (!bc_module_relay_set_mode(&module_relay, relay_mode == TASK_RELAY_MODE_FALSE ? BC_MODULE_RELAY_MODE_NO : BC_MODULE_RELAY_MODE_NC ))
            {
                bc_log_error("task_relay_worker: bc_module_relay_set_mode");
            }

        }

        if (last_mode!=relay_mode)
        {
            bc_talk_publish_relay((int)relay_mode, self->_device_address);
            last_mode = relay_mode;
            bc_os_task_sleep(1000L - (bc_tick_get() - self->_tick_last_feed)); //TODO michal navrhuje cvak max 1 za sekundu
        }
        else
        {
            bc_os_task_sleep(100L - (bc_tick_get() - self->_tick_last_feed)); //TODO pokud nedojde k zmene stavu povoluju 10krat za sekundu bliknout diodou
        }

    }

    return NULL;
}
