#include "task_co2.h"
#include "bc_log.h"
#include "bc_module_co2.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

void *task_co2_worker(void *task_parameter)
{
    int16_t value;
    bc_tick_t tick_feed_interval;
    bc_module_co2_t module_co2;

    task_worker_t *self = (task_worker_t *) task_parameter;

    bc_log_info("task_co2_worker: ");

    bc_i2c_interface_t interface;

    interface.bridge = self->_bridge;
    interface.channel = BC_BRIDGE_I2C_CHANNEL_0;

    if (!bc_module_co2_init(&module_co2, &interface))
    {
        bc_log_debug("task_co2_worker: bc_module_co2_init false");

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

        bc_log_debug("task_co2_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_co2_worker: quit_request");
            break;
        }

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
