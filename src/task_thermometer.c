#include "task_thermometer.h"
#include "bc_log.h"
#include "bc_tag_temperature.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "task.h"

static void *task_thermometer_worker(void *parameter);
static bool task_thermometer_is_quit_request(task_thermometer_t *self);

void task_thermometer_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_thermometer_t *self;

    bc_log_info("task_thermometer_spawn: spawning instance for bus %d, address 0x%02X",
                (uint8_t) task_info->i2c_channel, task_info->device_address);

    self = (task_thermometer_t *) malloc(sizeof(task_thermometer_t));

    memset(self, 0, sizeof(task_thermometer_t));

    self->_i2c_interface.bridge = bridge;
    self->_i2c_interface.channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->_tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_thermometer_worker, self);

    bc_log_info("task_thermometer_spawn: spawned instance for bus %d, address 0x%02X",
                (uint8_t) task_info->i2c_channel, task_info->device_address);

    task_info->task = self;
    task_info->enabled = true;
}

void task_thermometer_terminate(task_thermometer_t *self)
{
    bc_bridge_i2c_channel_t i2c_channel;
    uint8_t device_address;

    i2c_channel = (uint8_t) self->_i2c_interface.channel;
    device_address = self->_device_address;

    bc_log_info("task_thermometer_terminate: terminating instance for bus %d, address 0x%02X",
                i2c_channel, device_address);

    bc_os_mutex_lock(&self->mutex);
    self->_quit = true;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);

    bc_os_task_destroy(&self->task);
    bc_os_semaphore_destroy(&self->semaphore);
    bc_os_mutex_destroy(&self->mutex);

    free(self);

    bc_log_info("task_thermometer_terminate: terminated instance for bus %d, address 0x%02X",
                i2c_channel, device_address);
}

void task_thermometer_set_interval(task_thermometer_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->_tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_thermometer_worker(void *parameter)
{
    task_thermometer_t *self;

    bool init_ok = false;

    bc_tag_temperature_t tag_temperature;
    self = (task_thermometer_t *) parameter;

    bc_log_info("task_thermometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_interface.channel, self->_device_address);



    while (true)
    {

        bc_tag_temperature_state_t state;
        bc_tick_t tick_feed_interval;

        bool valid;
        float value;

        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->_tick_feed_interval;
        bc_os_mutex_unlock(&self->mutex);

        bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);

        bc_log_debug("task_thermometer_worker: wake up signal");


        if (init_ok==false) //TODO predelat do task manageru
        {
            if (!bc_tag_temperature_init(&tag_temperature, &self->_i2c_interface, self->_device_address))
            {
                bc_log_error("task_thermometer_worker: bc_tag_temperature_init");
                continue;
            }

            if (!bc_tag_temperature_single_shot_conversion(&tag_temperature))
            {
                bc_log_error("task_thermometer_worker: bc_tag_temperature_single_shot_conversion");
                continue;
            }

            init_ok = true;
        }


        if (task_thermometer_is_quit_request(self))
        {
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        valid = true;

        if (!bc_tag_temperature_get_state(&tag_temperature, &state))
        {
            bc_log_error("task_thermometer_worker: bc_tag_temperature_get_state");
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
                    char topic[32];

                    bc_log_info("task_thermometer_worker: temperature = %.1f C", value);

                    snprintf(topic, sizeof(topic), "thermometer/i2c%d-%02x", (uint8_t) self->_i2c_interface.channel, self->_device_address);

                    bc_talk_publish_begin(topic);
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
                    continue;
                }

                break;
            }
            default:
            {
                if (!bc_tag_temperature_power_down(&tag_temperature))
                {
                    bc_log_error("task_thermometer_worker: bc_tag_temperature_power_down");
                    continue;
                }

                break;
            }
        }
    }

    return NULL;
}

static bool task_thermometer_is_quit_request(task_thermometer_t *self)
{
    bc_os_mutex_lock(&self->mutex);

    if (self->_quit)
    {
        bc_os_mutex_unlock(&self->mutex);

        return true;
    }

    bc_os_mutex_unlock(&self->mutex);

    return false;
}
