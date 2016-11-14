#include "task_barometer.h"
#include "bc_log.h"
#include "bc_tag_barometer.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

void *task_barometer_worker(void *task_parameter)
{
    float altitude = 0;
    float absolute_pressure = 0;
    bool altitude_valid = false;
    bool absolute_pressure_valid = false;
    bc_tick_t one_shot_conversion = 0;

    bc_tick_t tick_feed_interval;
    bc_tag_barometer_state_t state;

    task_worker_t *self = (task_worker_t *) task_parameter;

    bc_log_info("task_barometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_barometer_t tag_barometer;

    if (!bc_tag_barometer_init(&tag_barometer, &interface, self->_device_address))
    {
        bc_log_debug("task_barometer_worker: bc_tag_barometer_init false bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);

        return NULL;
    }

    task_worker_set_init_done(self);

    while (true)
    {
        task_worker_get_interval(self, &tick_feed_interval);

        //bc_log_debug("task_barometer_worker: tick_feed_interval %d ", tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        bc_log_debug("task_barometer_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_barometer_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_barometer_get_state(&tag_barometer, &state))
        {
            bc_log_error("task_barometer_worker: bc_tag_barometer_get_state");
            return NULL;
        }

        //bc_log_debug("task_barometer_worker: state=%d", state);

        switch (state)
        {
            case BC_TAG_BAROMETER_STATE_POWER_DOWN:
            {
                altitude_valid = false;
                absolute_pressure_valid = false;

                if (!bc_tag_barometer_one_shot_conversion_altitude(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_altitude");
                    return NULL;
                }

                one_shot_conversion = self->_tick_last_feed;

                break;
            }
            case BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE:
            {
                altitude_valid = false;


                if (!bc_tag_barometer_get_altitude(&tag_barometer, &altitude))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_get_altitude");
                    return NULL;
                }

                altitude_valid = true;

                if (!bc_tag_barometer_one_shot_conversion_pressure(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_pressure");
                    return NULL;
                }

                one_shot_conversion = self->_tick_last_feed;

                break;
            }
            case BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE:
            {

                absolute_pressure_valid = false;


                if (!bc_tag_barometer_get_pressure(&tag_barometer, &absolute_pressure))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_get_pressure");
                    return NULL;
                }

                absolute_pressure /= 1000.f;
                absolute_pressure_valid = true;

                if (!bc_tag_barometer_one_shot_conversion_altitude(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_altitude");
                    return NULL;
                }

                one_shot_conversion = self->_tick_last_feed;

                break;
            }
            case BC_TAG_BAROMETER_STATE_CONVERSION:
            {
                if ( (one_shot_conversion + 3000) < self->_tick_last_feed )
                {
                    if (!bc_tag_barometer_reset_and_power_down(&tag_barometer))
                    {
                        bc_log_error("task_barometer_worker: bc_tag_barometer_reset_and_power_down");
                    }

                    return NULL;
                }
            }
            default:
            {
                break;
            }
        }

        if (altitude_valid && absolute_pressure_valid)
        {
            bc_talk_publish_begin_auto((uint8_t) self->_i2c_channel, self->_device_address);
            bc_talk_publish_add_quantity("altitude", "m", "%0.1f", altitude);
            bc_talk_publish_add_quantity("pressure", "kPa", "%0.3f", absolute_pressure);
            bc_talk_publish_end();
        }

    }

    return NULL;
}
