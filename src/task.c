#include "task.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "task_led.h"
#include "task_barometer.h"
#include "task_humidity.h"
#include "bc_log.h"

bool task_is_alive(task_info_t *task_info)
{
    if (task_info->worker == NULL)
    {
        return false;
    }
    return bc_os_task_is_alive(&task_info->worker->task);
}

bool task_is_alive_and_init_done(task_info_t *task_info)
{
    bool init_done;

    if (!task_is_alive(task_info))
    {
        return false;
    }

    task_lock(task_info);
    init_done = task_info->worker->init_done;
    task_unlock(task_info);

    return init_done;
}

void task_lock(task_info_t *task_info)
{
    bc_os_mutex_lock(task_info->mutex);
}

void task_unlock(task_info_t *task_info)
{
    bc_os_mutex_unlock(task_info->mutex);
}

void task_semaphore_put(task_info_t *task_info)
{
    task_lock(task_info);
    if (task_info->worker != NULL)
    {
        bc_os_semaphore_put(&task_info->worker->semaphore);
    }
    else
    {
        bc_log_error("task_semaphore_put task_info->worker==NULL bus %d, address 0x%02X",
                     (uint8_t) task_info->i2c_channel, task_info->device_address);
    }
    task_unlock(task_info);
}

bool task_set_interval(task_info_t *task_info, bc_tick_t interval)
{
    if (task_info->type == TASK_TYPE_MODULE_CO2)
    {
        if ((interval < 16) && (interval > 0))
        {
            return false;
        }
    }

    task_lock(task_info);
    *task_info->publish_interval = interval;
    task_unlock(task_info);

    task_semaphore_put(task_info);

    return true;
}

void task_get_interval(task_info_t *task_info, bc_tick_t *interval)
{
    task_lock(task_info);
    *interval = *task_info->publish_interval;
    task_unlock(task_info);
}

bool task_worker_is_quit_request(task_worker_t *self)
{
    bc_os_mutex_lock(self->mutex);

    if (self->_quit)
    {
        bc_os_mutex_unlock(self->mutex);

        return true;
    }

    bc_os_mutex_unlock(self->mutex);

    return false;
}

void task_worker_get_interval(task_worker_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(self->mutex);

    *interval = *self->publish_interval * 1000;

    bc_os_mutex_unlock(self->mutex);
}

void task_worker_set_init_done(task_worker_t *self)
{
    bc_os_mutex_lock(self->mutex);

    self->init_done = true;

    bc_os_mutex_unlock(self->mutex);
}