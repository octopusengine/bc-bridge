#include "task_lux_meter.h"
#include "bc_log.h"
#include "bc_tag_lux_meter.h"
#include "application_out.h"
#include "bc_tag.h"
#include "bc_bridge.h"

static void *task_lux_meter_worker(void *parameter);

task_lux_meter_t *task_lux_meter_spawn(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address)
{
    task_lux_meter_t *self = (task_lux_meter_t *) malloc(sizeof(task_lux_meter_t));

    self->_bridge = bridge;
    self->_i2c_channel = i2c_channel;
    self->_device_address = device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_lux_meter_worker, self);

    return self;
}

void task_lux_meter_set_interval(task_lux_meter_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_lux_meter_worker(void *parameter)
{
    float value;
    bc_tick_t tick_feed_interval;
    bc_tag_lux_meter_state_t state;

    task_lux_meter_t *self = (task_lux_meter_t *) parameter;

    bc_log_info("task_lux_meter_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_tag_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_lux_meter_t tag_lux_meter;

    bc_tag_lux_meter_init(&tag_lux_meter, &interface, self->_device_address);

    while (true)
    {
        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->tick_feed_interval;
        bc_os_mutex_unlock(&self->mutex);

        bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);

        bc_log_debug("task_lux_meter_worker: wake up signal");

        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_lux_meter_get_state(&tag_lux_meter, &state))
        {
            continue;
        }

        switch (state)
        {
            case BC_TAG_LUX_METER_STATE_POWER_DOWN:
            {
                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    continue;
                }
                break;
            }
            case BC_TAG_LUX_METER_STATE_CONVERSION:
            {
                break;
            }
            case BC_TAG_LUX_METER_STATE_RESULT_READY:
            {
                if (!bc_tag_lux_meter_read_result(&tag_lux_meter))
                {
                    continue;
                }

                if (!bc_tag_lux_meter_get_result_lux(&tag_lux_meter, &value))
                {
                    continue;
                }

                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    continue;
                }

                bc_log_info("task_lux_meter_worker: temperature = %.1f C", value);
                application_out_write("[\"lux-meter\", {\"0/illuminance\": [%0.2f, \"lux\"]}]", value);
                break;
            }
            default:
            {
                break;
            }
        }

    }

    return NULL;
}
