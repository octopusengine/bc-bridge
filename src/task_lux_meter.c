#include "task_lux_meter.h"
#include "bc_log.h"
#include "bc_tag_lux_meter.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_lux_meter_worker(void *parameter);

void task_lux_meter_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_lux_meter_t *self = (task_lux_meter_t *) malloc(sizeof(task_lux_meter_t));

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_lux_meter_worker, self);

    task_info->task = self;
    task_info->enabled = true;
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
    char topic[32];

    task_lux_meter_t *self = (task_lux_meter_t *) parameter;

    bc_log_info("task_lux_meter_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    snprintf(topic, sizeof(topic), "lux-meter/i2c%d-%02x", (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_lux_meter_t tag_lux_meter;

    if (!bc_tag_lux_meter_init(&tag_lux_meter, &interface, self->_device_address))
    {
        bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_init");
    }

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
            bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_get_state");
            continue;
        }

        switch (state)
        {
            case BC_TAG_LUX_METER_STATE_POWER_DOWN:
            {
                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_single_shot_conversion");
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
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_read_result");
                    continue;
                }

                if (!bc_tag_lux_meter_get_result_lux(&tag_lux_meter, &value))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_get_result_lux");
                    continue;
                }

                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_single_shot_conversion");
                    continue;
                }

                bc_log_info("task_lux_meter_worker: illuminance = %.1f lux", value);
                bc_talk_publish_begin(topic);
                bc_talk_publish_add_quantity("illuminance", "lux", "%0.2f", value);
                bc_talk_publish_end();
                //application_out_write("[\"lux-meter\", {\"0/illuminance\": [%0.2f, \"lux\"]}]", value);
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
