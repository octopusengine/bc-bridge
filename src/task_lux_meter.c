#include "task_lux_meter.h"
#include "bc_log.h"
#include "bc_tag_lux_meter.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

void *task_lux_meter_worker(void *task_parameter)
{
    task_worker_t *self = (task_worker_t *) task_parameter;
    float value;
    bc_tick_t tick_feed_interval;
    bc_tag_lux_meter_state_t state;

    bc_log_info("task_lux_meter_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_lux_meter_t tag_lux_meter;

    if (!bc_tag_lux_meter_init(&tag_lux_meter, &interface, self->_device_address))
    {
        bc_log_debug("task_lux_meter_worker: bc_tag_lux_meter_init false bus %d, address 0x%02X",
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

        bc_log_debug("task_lux_meter_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_lux_meter_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_lux_meter_get_state(&tag_lux_meter, &state))
        {
            bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_get_state");
            return NULL;
        }

        switch (state)
        {
            case BC_TAG_LUX_METER_STATE_POWER_DOWN:
            {
                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_single_shot_conversion");
                    return NULL;
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
                    return NULL;
                }

                if (!bc_tag_lux_meter_get_result_lux(&tag_lux_meter, &value))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_get_result_lux");
                    return NULL;
                }

                if (!bc_tag_lux_meter_single_shot_conversion(&tag_lux_meter))
                {
                    bc_log_error("task_lux_meter_worker: bc_tag_lux_meter_single_shot_conversion");
                    return NULL;
                }

                bc_log_info("task_lux_meter_worker: illuminance = %.1f lux", value);
                bc_talk_publish_begin_auto((uint8_t) self->_i2c_channel, self->_device_address);
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
