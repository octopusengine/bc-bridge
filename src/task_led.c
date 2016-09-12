#include "task_led.h"
#include "bc_log.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_led_worker(void *parameter);

void task_led_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_led_t *self = (task_led_t *) malloc(sizeof(task_led_t));

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;

    bc_os_mutex_init(&self->mutex);
    bc_os_semaphore_init(&self->semaphore, 0);
    bc_os_task_init(&self->task, task_led_worker, self);

    task_info->task = self;
    task_info->enabled = true;
}

void task_led_set_interval(task_led_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->tick_feed_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}


void task_led_set_state(task_led_t *self, task_led_state_t state)
{
    bc_os_mutex_lock(&self->mutex);
    self->state = state;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

static void *task_led_worker(void *parameter)
{
    bc_tick_t tick_feed_interval;
    task_led_state_t last_state;
    task_led_state_t state;
    bc_bridge_led_t bridge_led_state;
    int blink_cnt = 0;

    task_led_t *self = (task_led_t *) parameter;

    bc_log_info("task_led_worker: started instance ");

    bc_os_mutex_lock(&self->mutex);
    bc_bridge_led_get(self->_bridge, &bridge_led_state);
    if (bridge_led_state==BC_BRIDGE_LED_ON)
    {
        self->state = TASK_LED_ON;
    }
    else
    {
        self->state = TASK_LED_OFF;
    }
    last_state = self->state;
    bc_os_mutex_unlock(&self->mutex);

    while (true)
    {
        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->tick_feed_interval;
        state = self->state;
        bc_os_mutex_unlock(&self->mutex);

        if ((state==TASK_LED_OFF) || (state==TASK_LED_ON))
        {
            bc_os_semaphore_get(&self->semaphore);
        }
        else
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }

        self->_tick_last_feed = bc_tick_get();

        bc_os_mutex_lock(&self->mutex);
        state = self->state;
        bc_os_mutex_unlock(&self->mutex);

        bc_log_debug("task_led_worker: wake up signal");

        if (last_state != state)
        {
            switch (state)
            {
                case TASK_LED_OFF :
                {
                    bc_bridge_led_set(self->_bridge, BC_BRIDGE_LED_OFF);
                    last_state = state;
                    continue;
                }
                case TASK_LED_ON :
                {
                    bc_bridge_led_set(self->_bridge, BC_BRIDGE_LED_ON);
                    last_state = state;
                    continue;
                }
                case TASK_LED_1DOT :
                {
                    blink_cnt = 2;
                    break;
                }
                case TASK_LED_2DOT :
                {
                    blink_cnt = 4;
                    break;
                }
                case TASK_LED_3DOT :
                {
                    blink_cnt = 6;
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

        if (blink_cnt <= 0)
        {
            bc_os_mutex_lock(&self->mutex);
            self->state = TASK_LED_OFF;
            last_state = TASK_LED_OFF;
            bc_os_mutex_unlock(&self->mutex);
            continue;
        }

        if ((blink_cnt-- % 2) == 0)
        {
            bc_bridge_led_set(self->_bridge, BC_BRIDGE_LED_ON);
        }
        else
        {
            bc_bridge_led_set(self->_bridge, BC_BRIDGE_LED_OFF);
        }

    }

    return NULL;
}
