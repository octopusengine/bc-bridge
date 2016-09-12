#include "task_barometer.h"
#include "bc_log.h"
#include "bc_tag_barometer.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_barometer_worker(void *parameter);

void task_barometer_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_barometer_t *self = (task_barometer_t *) malloc(sizeof(task_barometer_t));

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_barometer_worker, self);

    task_info->task = self;
    task_info->enabled = true;
}

void task_barometer_set_interval(task_barometer_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_barometer_get_interval(task_barometer_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(&self->mutex);
    *interval = self->tick_feed_interval;
    bc_os_mutex_unlock(&self->mutex);
}


static void *task_barometer_worker(void *parameter)
{
    float altitude;
    float absolute_pressure;
    bool altitude_valid = false;
    bool absolute_pressure_valid = false;

    bool init_ok;
    bc_tick_t tick_feed_interval;
    bc_tag_barometer_state_t state;

    task_barometer_t *self = (task_barometer_t *) parameter;

    bc_log_info("task_barometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_barometer_t tag_barometer;



    while (true)
    {
        task_barometer_get_interval(self, &tick_feed_interval);

        bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);

        bc_log_debug("task_barometer_worker: wake up signal");

        if (init_ok==false) //TODO predelat do task manageru
        {
            if (!bc_tag_barometer_init(&tag_barometer, &interface))
            {
                bc_log_error("task_barometer_worker: bc_tag_barometer_init");
                continue;
            }
            init_ok = true;
        }


        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_barometer_get_state(&tag_barometer, &state))
        {
            bc_log_error("task_barometer_worker: bc_tag_barometer_get_state");
            continue;
        }

        switch (state)
        {
            case BC_TAG_BAROMETER_STATE_POWER_DOWN:
            {
                altitude_valid = false;
                absolute_pressure_valid = false;

                if (!bc_tag_barometer_one_shot_conversion_altitude(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_altitude");
                    continue;
                }

                break;
            }
            case BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE:
            {
                altitude_valid = false;

                if (!bc_tag_barometer_read_result(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_read_result");
                    continue;
                }

                if (!bc_tag_barometer_get_altitude(&tag_barometer, &altitude))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_get_altitude");
                    continue;
                }

                altitude_valid = true;

                if (!bc_tag_barometer_one_shot_conversion_pressure(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_pressure");
                    continue;
                }

                break;
            }
            case BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE:
            {

                absolute_pressure_valid = false;

                if (!bc_tag_barometer_read_result(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_read_result");
                    continue;
                }

                if (!bc_tag_barometer_get_pressure(&tag_barometer, &absolute_pressure))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_get_pressure");
                    continue;
                }

                absolute_pressure /= 1000.f;
                absolute_pressure_valid = true;

                if (!bc_tag_barometer_one_shot_conversion_altitude(&tag_barometer))
                {
                    bc_log_error("task_barometer_worker: bc_tag_barometer_one_shot_conversion_altitude");
                    continue;
                }

                break;
            }
            case BC_TAG_BAROMETER_STATE_CONVERSION:
            {
                break;
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
