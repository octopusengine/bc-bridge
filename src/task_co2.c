#include "task_co2.h"
#include "bc_log.h"
#include "bc_module_co2.h"
#include "application_out.h"
#include "bc_tag.h"
#include "bc_bridge.h"

static void *task_co2_worker(void *parameter);

task_co2_t *task_co2_spawn(bc_bridge_t *bridge)
{
    bc_log_info("task_co2_spawn: ");

    task_co2_t *self = (task_co2_t *) malloc(sizeof(task_co2_t));

    self->_bridge = bridge;

    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_co2_worker, self);

    return self;
}

void task_co2_set_interval(task_co2_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_co2_worker(void *parameter)
{
    int16_t value;
    bc_tick_t tick_feed_interval;
    bc_module_co2_t module_co2;

    task_co2_t *self = (task_co2_t *) parameter;

    bc_log_info("task_co2_worker: ");

    bc_tag_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = BC_BRIDGE_I2C_CHANNEL_0;

    bc_module_co2_init(&module_co2, &interface);

    while (true)
    {
        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->tick_feed_interval;
        bc_os_mutex_unlock(&self->mutex);

        bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);

        bc_log_debug("task_co2_worker: wake up signal");

        self->_tick_last_feed = bc_tick_get();

        bc_module_co2_task(&module_co2);

        if (bc_module_co2_get_concentration(&module_co2, &value))
        {
            bc_log_info("task_co2_worker: temperature = %.1f C", value);
            application_out_write("[\"co2-sensor/0\", {\"concentration\": [%d, \"ppm\"]}]", value);
        }

    }

    return NULL;
}
