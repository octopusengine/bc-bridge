#include "task_co2.h"
#include "bc_log.h"
#include "bc_module_co2.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_co2_worker(void *parameter);
static bool task_co2_is_quit_request(task_co2_t *self);

void task_co2_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    bc_log_info("task_co2_spawn: ");

    task_co2_t *self = (task_co2_t *) malloc(sizeof(task_co2_t));

    if (self == NULL)
    {
        bc_log_fatal("task_co2_spawn: call failed: malloc");
    }

    self->_bridge = bridge;
    self->_quit = false;

    self->tick_feed_interval = BC_MODULE_CO2_MINIMAL_MEASUREMENT_INTERVAL_MS;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_co2_worker, self);

    task_info->task = self;
    task_info->enabled = true;
}

void task_co2_set_interval(task_co2_t *self, bc_tick_t interval)
{
    if ( (interval<BC_MODULE_CO2_MINIMAL_MEASUREMENT_INTERVAL_MS) && (interval>0) )
    {
        return;
    }
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_co2_get_interval(task_co2_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(&self->mutex);
    *interval = self->tick_feed_interval;
    bc_os_mutex_unlock(&self->mutex);

}

void task_co2_terminate(task_co2_t *self)
{

    bc_log_info("task_co2_terminate: terminating instance ");

    bc_os_mutex_lock(&self->mutex);
    self->_quit = true;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);

    bc_os_task_destroy(&self->task);
    bc_os_semaphore_destroy(&self->semaphore);
    bc_os_mutex_destroy(&self->mutex);

    free(self);

    bc_log_info("task_co2_terminate: terminated instance ");
}

static void *task_co2_worker(void *parameter)
{
    int16_t value;
    bc_tick_t tick_feed_interval;
    bc_module_co2_t module_co2;

    task_co2_t *self = (task_co2_t *) parameter;

    bc_log_info("task_co2_worker: ");

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = BC_BRIDGE_I2C_CHANNEL_0;

    if (!bc_module_co2_init(&module_co2, &interface))
    {
        bc_log_debug("task_co2_worker: bc_module_co2_init false");
        bc_os_mutex_lock(&self->mutex);
        self->_quit = true;
        bc_os_mutex_unlock(&self->mutex);
        return NULL;
    }

    while (true)
    {
        task_co2_get_interval(self, &tick_feed_interval);

        if (tick_feed_interval < 0)
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        if (task_co2_is_quit_request(self))
        {
            break;
        }

        bc_log_debug("task_co2_worker: wake up signal");

        self->_tick_last_feed = bc_tick_get();

        bc_module_co2_task(&module_co2);

        if (bc_module_co2_get_concentration(&module_co2, &value))
        {

            bc_log_info("task_co2_worker: concentration = %d ppm", value);
            bc_talk_publish_begin("co2-sensor/i2c0-38");
            bc_talk_publish_add_quantity("concentration", "ppm", "%d", value);
            bc_talk_publish_end();

        }

    }

    return NULL;
}

static bool task_co2_is_quit_request(task_co2_t *self)
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