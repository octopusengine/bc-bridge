#include "task_thermometer.h"
#include "bc_log.h"
#include "bc_tag_temperature.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "task.h"

void *task_thermometer_worker(void *task_parameter)
{
    task_worker_t *self = (task_worker_t *) task_parameter;

    bc_tag_temperature_t tag_temperature;

    bc_i2c_interface_t interface;


    bc_log_info("task_thermometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    if (!bc_tag_temperature_init(&tag_temperature, &interface, self->_device_address))
    {
        bc_log_debug("task_thermometer_worker: bc_tag_temperature_init false bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);

        return NULL;
    }

    task_worker_set_init_done(self);

    if (!bc_tag_temperature_single_shot_conversion(&tag_temperature))
    {
        bc_log_error("task_thermometer_worker: bc_tag_temperature_single_shot_conversion false bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);

        return NULL;
    }

    while (true)
    {

        bc_tag_temperature_state_t state;
        bc_tick_t tick_feed_interval;

        bool valid;
        float value;

        task_worker_get_interval(self, &tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        bc_log_debug("task_thermometer_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_thermometer_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        valid = true;

        if (!bc_tag_temperature_get_state(&tag_temperature, &state))
        {
            bc_log_error("task_thermometer_worker: bc_tag_temperature_get_state");
            return NULL;
        }

        switch (state)
        {
            case BC_TAG_TEMPERATURE_STATE_POWER_DOWN:
            {
                if (!bc_tag_temperature_read_temperature(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_read_temperature");

                    return NULL;
                }

                if (!bc_tag_temperature_get_temperature_celsius(&tag_temperature, &value))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_get_temperature_celsius");

                    return NULL;
                }

                if (!bc_tag_temperature_single_shot_conversion(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: call failed: bc_tag_temperature_single_shot_conversion");

                    return NULL;
                }

                if (valid)
                {

                    bc_log_info("task_thermometer_worker: temperature = %.1f C", value);

                    bc_talk_publish_begin_auto((uint8_t) self->_i2c_channel, self->_device_address);
                    bc_talk_publish_add_quantity("temperature", "\\u2103", "%0.2f", value);
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
                if (!bc_tag_temperature_power_down(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: bc_tag_temperature_power_down");
                    return NULL;
                }

                break;
            }
            default:
            {
                if (!bc_tag_temperature_power_down(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: bc_tag_temperature_power_down");
                    return NULL;
                }

                break;
            }
        }
    }

    return NULL;
}

