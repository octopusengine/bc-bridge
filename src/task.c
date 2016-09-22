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
    if (task_info->worker == NULL )
    {
        return false;
    }
    return  bc_os_task_is_alive(&task_info->worker->task);
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
    task_unlock(task_info);
}

void task_set_interval(task_info_t *task_info, bc_tick_t interval)
{
    if (task_info->type == TASK_TYPE_MODULE_CO2)
    {
        if ((interval < 16000) && (interval > 0))
        {
            return;
        }
    }

    task_lock(task_info);
    *task_info->publish_interval = interval;
    task_unlock(task_info);

    task_semaphore_put(task_info);

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
    *interval = *self->publish_interval;
    bc_os_mutex_unlock(self->mutex);
}