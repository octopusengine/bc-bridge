#include "task_thermometer.h"
#include "bc_log.h"
#include "bc_tag_temperature.h"
#include "bc_talk.h"
#include "bc_tag.h"
#include "bc_bridge.h"

static void *task_thermometer_worker(void *parameter);

task_thermometer_t *task_thermometer_spawn(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address)
{
    task_thermometer_t *self = (task_thermometer_t *) malloc(sizeof(task_thermometer_t));

    self->_bridge = bridge;
    self->_i2c_channel = i2c_channel;
    self->_device_address = device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_thermometer_worker, self);

    return self;
}

void task_thermometer_set_interval(task_thermometer_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_thermometer_worker(void *parameter)
{
    bool valid;
    float value;
    char topic[20];

    bc_tick_t tick_feed_interval;
    bc_tag_temperature_state_t state;

    task_thermometer_t *self = (task_thermometer_t *) parameter;

    bc_log_info("task_thermometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    sprintf(&topic, "thermometer/i2c%d-%02X", (uint8_t) self->_i2c_channel, self->_device_address);

    bc_tag_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_temperature_t tag_temperature;

    bc_tag_temperature_init(&tag_temperature, &interface, self->_device_address);

    bc_tag_temperature_single_shot_conversion(&tag_temperature);

    while (true)
    {
        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->tick_feed_interval;
        bc_os_mutex_unlock(&self->mutex);

        bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);

        bc_log_debug("task_thermometer_worker: wake up signal");

        self->_tick_last_feed = bc_tick_get();

        valid = true;

        if (!bc_tag_temperature_get_state(&tag_temperature, &state))
        {
            continue;
        }

        switch (state)
        {
            case BC_TAG_TEMPERATURE_STATE_POWER_DOWN:
            {
                if (!bc_tag_temperature_read_temperature(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_read_temperature");

                    continue;
                }

                if (!bc_tag_temperature_get_temperature_celsius(&tag_temperature, &value))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_get_temperature_celsius");

                    continue;
                }

                if (!bc_tag_temperature_single_shot_conversion(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_single_shot_conversion");

                    continue;
                }

                if (valid)
                {
                    bc_log_info("task_thermometer_worker: temperature = %.1f C", value);
                    bc_talk_publish_begin(&topic);
                    bc_talk_publish_add_quantity_final("temperature", "\\u2103", "%0.2f", value);
                    bc_talk_publish_end();
                }
                else
                {
                    bc_log_info("task_thermometer_worker: temperature = ?", value);
                }

                valid = false;

                break;
            }
            case BC_TAG_TEMPERATURE_STATE_CONVERSION:
            {
                bc_tag_temperature_power_down(&tag_temperature);

                break;
            }
            default:
            {
                bc_tag_temperature_power_down(&tag_temperature);

                break;
            }
        }
    }

    return NULL;
}
