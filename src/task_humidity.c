#include "task_humidity.h"
#include "bc_log.h"
#include "bc_tag_humidity.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

void *task_humidity_worker(void *task_parameter)
{
    float value;
    bc_tick_t tick_feed_interval;
    bc_tag_humidity_state_t state;

    task_worker_t *self = (task_worker_t *) task_parameter;

    bc_log_info("task_humidity_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_humidity_t tag_humidity;

    if (!bc_tag_humidity_init(&tag_humidity, &interface, self->_device_address))
    {
        bc_log_debug("task_humidity_worker: bc_tag_humidity_init false for bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);

        return NULL;
    }

    task_worker_set_init_done(self);

    while (true)
    {
        task_worker_get_interval(self, &tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        bc_log_debug("task_humidity_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_humidity_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_humidity_get_state(&tag_humidity, &state))
        {
            bc_log_error("task_humidity_worker: bc_tag_humidity_get_state");
            return NULL;
        }

        switch (state)
        {
            case BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ:
            {

                if (!bc_tag_humidity_load_calibration(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_load_calibration");
                    return NULL;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_POWER_DOWN:
            {
                if (!bc_tag_humidity_power_up(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_power_up");
                    return NULL;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_POWER_UP:
            {
                if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_one_shot_conversion");
                    return NULL;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_CONVERSION:
            {
                break;
            }
            case BC_TAG_HUMIDITY_STATE_RESULT_READY:
            {

                if (!bc_tag_humidity_get_relative_humidity(&tag_humidity, &value))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_get_result");
                    return NULL;
                }

                if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_one_shot_conversion");
                    return NULL;
                }

                bc_talk_publish_begin_auto((uint8_t) self->_i2c_channel, self->_device_address);
                bc_talk_publish_add_quantity("relative-humidity", "%", "%0.1f", value);
                bc_talk_publish_end();
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
