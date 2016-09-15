#include "task_led.h"
#include "bc_log.h"
#include "bc_talk.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

static void *task_led_worker(void *parameter);
static bool task_led_is_quit_request(task_led_t *self);

void task_led_spawn(bc_bridge_t *bridge, task_info_t *task_info)
{
    task_led_t *self = (task_led_t *) malloc(sizeof(task_led_t));

    if (self == NULL)
    {
        bc_log_fatal("task_led_spawn: call failed: malloc");
    }

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->tick_feed_interval = 1000;
    self->blink_interval = 100;

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

void task_led_get_interval(task_led_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(&self->mutex);
    *interval = self->tick_feed_interval;
    bc_os_mutex_unlock(&self->mutex);
}

void task_led_set_blink_interval(task_led_t *self, bc_tick_t interval)
{
    bc_os_mutex_lock(&self->mutex);
    self->blink_interval = interval;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_led_get_blink_interval(task_led_t *self, bc_tick_t *interval)
{
    bc_os_mutex_lock(&self->mutex);
    *interval = self->blink_interval;
    bc_os_mutex_unlock(&self->mutex);
}


void task_led_set_state(task_led_t *self, task_led_state_t state)
{
    bc_os_mutex_lock(&self->mutex);
    self->_state = state;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);
}

void task_led_get_state(task_led_t *self, task_led_state_t *state)
{
    bc_os_mutex_lock(&self->mutex);
    *state = self->_state;
    bc_os_mutex_unlock(&self->mutex);

}

void task_led_terminate(task_led_t *self)
{

    bc_log_info("task_led_terminate: terminating instance ");

    bc_os_mutex_lock(&self->mutex);
    self->_quit = true;
    bc_os_mutex_unlock(&self->mutex);

    bc_os_semaphore_put(&self->semaphore);

    bc_os_task_destroy(&self->task);
    bc_os_semaphore_destroy(&self->semaphore);
    bc_os_mutex_destroy(&self->mutex);

    free(self);

    bc_log_info("task_led_terminate: terminated instance ");
}

static void *task_led_worker(void *parameter)
{
    bc_tick_t tick_feed_interval;
    bc_tick_t blink_interval;
    task_led_state_t last_state;
    task_led_state_t state;
    bc_bridge_led_state_t bridge_led_state;
    int blink_cnt = 0;

    task_led_t *self = (task_led_t *) parameter;

    bc_log_info("task_led_worker: started instance ");

    bc_os_mutex_lock(&self->mutex);
    bc_bridge_led_get_state(self->_bridge, &bridge_led_state);
    if (bridge_led_state==BC_BRIDGE_LED_STATE_ON)
    {
        self->_state = TASK_LED_ON;
    }
    else
    {
        self->_state = TASK_LED_OFF;
    }
    last_state = self->_state;
    bc_os_mutex_unlock(&self->mutex);

    while (true)
    {
        int i;

        bc_os_mutex_lock(&self->mutex);
        tick_feed_interval = self->tick_feed_interval;
        bc_os_mutex_unlock(&self->mutex);

        if (blink_cnt>0)
        {
            bc_os_semaphore_timed_get(&self->semaphore, tick_feed_interval);
        }
        else
        {
            bc_os_semaphore_get(&self->semaphore);
        }


        if (task_led_is_quit_request(self))
        {
            break;
        }

        self->_tick_last_feed = bc_tick_get();

        bc_os_mutex_lock(&self->mutex);
        state = self->_state;
        blink_interval = self->blink_interval;
        bc_os_mutex_unlock(&self->mutex);

        bc_log_debug("task_led_worker: wake up signal");

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

        for (i=0; i<blink_cnt; i++)
        {
            bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_ON);
            if (task_led_is_quit_request(self))
            {
                break;
            }
            bc_os_task_sleep(blink_interval);
            bc_bridge_led_set_state(self->_bridge, BC_BRIDGE_LED_STATE_OFF);
            if (task_led_is_quit_request(self))
            {
                break;
            }
            bc_os_task_sleep(blink_interval);
        }

    }

    return NULL;
}

static bool task_led_is_quit_request(task_led_t *self)
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
