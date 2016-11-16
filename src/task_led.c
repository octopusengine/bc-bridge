#include "task_led.h"
#include "bc_log.h"
#include "task.h"

void *task_led_worker(void *task_parameter)
{
    bc_tick_t tick_feed_interval;
    bc_tick_t blink_interval;
    task_led_state_t last_state = TASK_LED_OFF;
    task_led_state_t state;
    int blink_cnt = 0;

    task_worker_t *self = (task_worker_t *) task_parameter;
    task_led_parameters_t *parameters = (task_led_parameters_t *) self->parameters;

    bc_log_info("task_led_worker: started instance ");

    bc_os_mutex_lock(self->mutex);
    if (last_state != parameters->state)
    {
        bc_os_semaphore_put(&self->semaphore);
    }
    bc_os_mutex_unlock(self->mutex);

    task_worker_set_init_done(self);

    while (true)
    {
        int i;

        bc_os_mutex_lock(self->mutex);
        tick_feed_interval = self->_tick_feed_interval;
        bc_os_mutex_unlock(self->mutex);

        if (blink_cnt > 0)
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }
        else
        {
            bc_os_semaphore_get(&self->semaphore);
        }

        bc_log_debug("task_led_worker: wake up signal");

        if (task_worker_is_quit_request(self))
        {
            bc_log_debug("task_led_worker: quit_request");
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        bc_os_mutex_lock(self->mutex);
        state = parameters->state;
        blink_interval = parameters->blink_interval;
        bc_os_mutex_unlock(self->mutex);

        if (last_state != state)
        {
            switch (state)
            {
                case TASK_LED_OFF :
                {
                    bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_OFF);
                    blink_cnt = 0;
                    break;
                }
                case TASK_LED_ON :
                {
                    bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_ON);
                    blink_cnt = 0;
                    break;
                }
                case TASK_LED_1DOT :
                {
                    blink_cnt = 1;
                    break;
                }
                case TASK_LED_2DOT :
                {
                    blink_cnt = 2;
                    break;
                }
                case TASK_LED_3DOT :
                {
                    blink_cnt = 3;
                    break;
                }
                default:
                {
                    blink_cnt = 0;
                    continue;
                }
            }

            last_state = state;
        }

        for (i = 0; i < blink_cnt; i++)
        {
            bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_ON);
            if (task_worker_is_quit_request(self))
            {
                bc_log_debug("task_led_worker: quit_request");
                break;
            }
            bc_os_task_sleep(blink_interval);
            bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_OFF);
            if (task_worker_is_quit_request(self))
            {
                bc_log_debug("task_led_worker: quit_request");
                break;
            }
            bc_os_task_sleep(blink_interval);
        }

    }

    return NULL;
}

void task_led_set_state(task_info_t *task_info, task_led_state_t state)
{
    task_lock(task_info);
    ((task_led_parameters_t *) task_info->parameters)->state = state;
    task_unlock(task_info);

    task_semaphore_put(task_info);
}

void task_led_get_state(task_info_t *task_info, task_led_state_t *state)
{
    task_lock(task_info);
    *state = ((task_led_parameters_t *) task_info->parameters)->state;
    task_unlock(task_info);
}
