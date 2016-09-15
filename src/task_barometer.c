#include "task_barometer.h"
#include "bc_log.h"
#include "bc_tag_barometer.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_barometer_worker(void *parameter);
static bool task_barometer_is_quit_request(task_barometer_t *self);

void task_barometer_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_barometer_t *self = (task_barometer_t *) malloc(sizeof(task_barometer_t));

    if (self == NULL)
    {
        bc_log_fatal("task_barometer_spawn: call failed: malloc");
    }

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;
    self->_quit = false;

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

void task_barometer_terminate(task_barometer_t *self)
{

    bc_bridge_i2c_channel_t i2c_channel;
    uint8_t device_address;

    i2c_channel = (uint8_t) self->_i2c_channel;
    device_address = self->_device_address;

    bc_log_info("task_barometer_terminate: terminating instance for bus %d, address 0x%02X",
                i2c_channel, device_address);

    bc_os_mutex_lock(&self->mutex);
    self->_quit = true;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);

    bc_os_task_destroy(&self->task);
    bc_os_semaphore_destroy(&self->semaphore);
    bc_os_mutex_destroy(&self->mutex);

    free(self);

    bc_log_info("task_barometer_terminate: terminated instance for bus %d, address 0x%02X",
                i2c_channel, device_address);
}


static void *task_barometer_worker(void *parameter)
{
    float altitude;
    float absolute_pressure;
    bool altitude_valid = false;
    bool absolute_pressure_valid = false;

    bc_tick_t tick_feed_interval;
    bc_tag_barometer_state_t state;

    task_barometer_t *self = (task_barometer_t *) parameter;

    bc_log_info("task_barometer_worker: started instance for bus %d, address 0x%02X",
                (uint8_t) self->_i2c_channel, self->_device_address);

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    bc_tag_barometer_t tag_barometer;

    if (!bc_tag_barometer_init(&tag_barometer, &interface))
    {
        bc_log_debug("task_barometer_worker: bc_tag_barometer_init false bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);
        bc_os_mutex_lock(&self->mutex);
        self->_quit = true;
        bc_os_mutex_unlock(&self->mutex);
        return NULL;
    }

    while (true)
    {
        task_barometer_get_interval(self, &tick_feed_interval);

        //bc_log_debug("task_barometer_worker: tick_feed_interval %d ", tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        if (task_barometer_is_quit_request(self))
        {
            bc_log_debug("task_barometer_worker: quit_request");
            break;
        }

        bc_log_debug("task_barometer_worker: wake up signal");

        self->_tick_last_feed = bc_tick_get();

        if (!bc_tag_barometer_get_state(&tag_barometer, &state))
        {
            bc_log_error("task_barometer_worker: bc_tag_barometer_get_state");
            continue;
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
                    continue;
                }

                break;
            }
            case BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE:
            {
                altitude_valid = false;


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

static bool task_barometer_is_quit_request(task_barometer_t *self)
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
