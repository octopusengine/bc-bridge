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
    bc_tick_t tick_publish_interval;
    bc_tick_t tick_last_publish = 0;
    bc_tick_t tick_feed_interval_publish;
    bc_tick_t tick_next_calibration_request = 0;

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

    self->_tick_last_feed = bc_tick_get();

    while (true)
    {
        task_worker_get_interval(self, &tick_publish_interval);

        bc_module_co2_task_get_feed_interval(&module_co2, &tick_feed_interval);

        tick_feed_interval_publish = tick_publish_interval - (self->_tick_last_feed - tick_last_publish);

        if ((tick_feed_interval_publish > 0) && (tick_feed_interval_publish < tick_feed_interval))
        {
            tick_feed_interval = tick_feed_interval_publish;
        }

        if (tick_feed_interval > 0)
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        bc_log_debug("task_co2_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_co2_worker: quit_request");
            break;
        }

        bc_module_co2_task(&module_co2);

        self->_tick_last_feed = bc_tick_get();

        if (bc_module_co2_task_is_state_error(&module_co2))
        {
            return NULL;
        }

        if (bc_module_co2_task_get_concentration(&module_co2, &value) &&
            ((tick_last_publish + tick_publish_interval) < self->_tick_last_feed))
        {
            bc_talk_publish_begin("co2-sensor/i2c0-38");
            bc_talk_publish_add_quantity("concentration", "ppm", "%d", value);
            bc_talk_publish_end();

            tick_last_publish = self->_tick_last_feed;
        }

        if (tick_next_calibration_request < self->_tick_last_feed)
        {
            bc_module_co2_task_set_calibration_request(&module_co2, BC_MODULE_CO2_CALIBRATION_ABC_RF);
            tick_next_calibration_request = self->_tick_feed_interval + (7*24*3600*1000);
        }

    }

    return NULL;
}
