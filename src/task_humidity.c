#include "task_humidity.h"
#include "bc_log.h"
#include "bc_tag_humidity.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_humidity_worker(void *parameter);

void task_humidity_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_humidity_t *self = (task_humidity_t *) malloc(sizeof(task_humidity_t));

    if (self == NULL)
    {
        bc_log_fatal("task_humidity_spawn: call failed: malloc");
    }

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_humidity_worker, self);

    task_info->task = self;
    task_info->enabled = true;
}

void task_humidity_set_interval(task_humidity_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_humidity_get_interval(task_humidity_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(&self->mutex);
    *interval = self->tick_feed_interval;
    bc_os_mutex_unlock(&self->mutex);
}


static void *task_humidity_worker(void *parameter)
{
    float value;
    bool init_ok = false;
    bc_tick_t tick_feed_interval;
    bc_tag_humidity_state_t state;

    task_humidity_t *self = (task_humidity_t *) parameter;

    bc_log_info("task_humidity_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_humidity_t tag_humidity;

    while (true)
    {
        task_humidity_get_interval(self, &tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        bc_log_debug("task_humidity_worker: wake up signal");

        if (init_ok==false) //TODO predelat do task manageru
        {
            if (!bc_tag_humidity_init(&tag_humidity, &interface))
            {
                bc_log_error("task_humidity_worker: bc_tag_humidity_init");
                continue;
            }
            init_ok = true;
        }


        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_humidity_get_state(&tag_humidity, &state))
        {
            bc_log_error("task_humidity_worker: bc_tag_humidity_get_state");
            continue;
        }

        switch (state)
        {
            case BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ:
            {

                if (!bc_tag_humidity_read_calibration(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_read_calibration");
                    continue;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_POWER_DOWN:
            {
                if (!bc_tag_humidity_power_up(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_power_up");
                    continue;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_POWER_UP:
            {
                if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_one_shot_conversion");
                    continue;
                }

                break;
            }
            case BC_TAG_HUMIDITY_STATE_CONVERSION:
            {
                break;
            }
            case BC_TAG_HUMIDITY_STATE_RESULT_READY:
            {

                if (!bc_tag_humidity_read_result(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_read_result");
                    continue;
                }

                if (!bc_tag_humidity_get_result(&tag_humidity, &value))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_get_result");
                    continue;
                }

                if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
                {
                    bc_log_error("task_humidity_worker: bc_tag_humidity_one_shot_conversion");
                    continue;
                }

                bc_talk_publish_begin_auto((uint8_t) self->_i2c_channel, self->_device_address);
                bc_talk_publish_add_quantity("relative-humidity", "%", "%0.1f", value);
                bc_talk_publish_end();
                break;
            }
            default:
            {
                continue;
            }
        }

    }

    return NULL;
}
