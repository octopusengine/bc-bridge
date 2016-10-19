#include "task.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "task_led.h"
#include "task_barometer.h"
#include "task_humidity.h"
#include "task_display_oled.h"
#include "bc_log.h"

static void _task_spawn_worker(bc_bridge_t *bridge, task_info_t *task_info, void *(*task_worker)(void *) );
static void _task_worker_terminate(task_info_t *task_info);

void task_manager_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length)
{
    int i;
    void *parameters;

    for (i = 0; i < length; i++)
    {

        if (task_info_list[i].mutex == NULL)
        {
            task_info_list[i].worker = NULL;
            task_info_list[i].mutex = malloc(sizeof(bc_os_mutex_t));
            bc_os_mutex_init(task_info_list[i].mutex);
            task_info_list[i].publish_interval = malloc(sizeof(bc_tick_t));
            *task_info_list[i].publish_interval = task_info_list[i].type == TASK_TYPE_MODULE_CO2 ? 16 : 1; //in sec
        }
        else if (task_is_alive(&task_info_list[i]))
        {
            continue;
        }

        switch (task_info_list[i].type)
        {
            case TASK_TYPE_LED:
            {
                if (task_info_list[i].parameters == NULL)
                {
                    parameters = malloc(sizeof(task_led_parameters_t));
                    ((task_led_parameters_t *) parameters)->blink_interval = 100;
                    ((task_led_parameters_t *) parameters)->state = TASK_LED_OFF;
                    task_info_list[i].parameters = parameters;
                }
                _task_spawn_worker(bridge, &task_info_list[i], task_led_worker);
                break;
            }
            case TASK_TYPE_TAG_THERMOMETHER:
            {
                _task_spawn_worker(bridge, &task_info_list[i], task_thermometer_worker);
                break;
            }
            case TASK_TYPE_TAG_LUX_METER:
            {
                _task_spawn_worker(bridge, &task_info_list[i], task_lux_meter_worker);
                break;
            }
            case TASK_TYPE_TAG_BAROMETER:
            {
                _task_spawn_worker(bridge, &task_info_list[i], task_barometer_worker);
                break;
            }
            case TASK_TYPE_TAG_HUMIDITY:
            {
                _task_spawn_worker(bridge, &task_info_list[i], task_humidity_worker);
                break;
            }
            case TASK_TYPE_MODULE_RELAY:
            {
                if (task_info_list[i].parameters == NULL)
                {
                    parameters = malloc(sizeof(task_relay_parameters_t));
                    ((task_relay_parameters_t *) parameters)->state = TASK_RELAY_STATE_NULL;
                    task_info_list[i].parameters = parameters;
                }
                _task_spawn_worker(bridge, &task_info_list[i], task_relay_worker);
                break;
            }
            case TASK_TYPE_MODULE_CO2:
            {
                _task_spawn_worker(bridge, &task_info_list[i], task_co2_worker);
                break;
            }
            case TASK_TYPE_DISPLAY_OLED:
            {
                if (task_info_list[i].parameters == NULL)
                {
                    parameters = malloc(sizeof(task_display_oled_parameters_t));
                    memset(parameters, 0x00, sizeof(task_display_oled_parameters_t) );
                    strcpy( ((task_display_oled_parameters_t *) parameters)->lines[0], "BigClown");
                    strcpy( ((task_display_oled_parameters_t *) parameters)->lines[2], "i2c oled display");
                    task_info_list[i].parameters = parameters;
                }
                _task_spawn_worker(bridge, &task_info_list[i], task_display_oled);
                break;
            }
            default:
                break;
        }

    }
}

void task_manager_terminate(task_info_t *task_info_list, size_t length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        _task_worker_terminate(&task_info_list[i]);
    }
}

static void _task_spawn_worker(bc_bridge_t *bridge, task_info_t *task_info, void *(*task_worker)(void *) )
{
    task_worker_t *self;

    bc_log_info("_task_spawn_worker: spawning instance for bus %d, address 0x%02X",
                (uint8_t) task_info->i2c_channel, task_info->device_address);

    self = (task_worker_t *) malloc(sizeof(task_worker_t));

    if (self == NULL)
    {
        bc_log_fatal("_task_spawn_worker: call failed: malloc");
    }

    memset(self, 0, sizeof(task_worker_t));
    self->_quit = false;
    self->init_done = false;
    bc_os_semaphore_init(&self->semaphore, 0);

    self->_bridge = bridge;
    self->_i2c_channel = task_info->i2c_channel;
    self->_device_address = task_info->device_address;
    self->mutex = task_info->mutex;
    self->parameters = task_info->parameters;
    self->publish_interval = task_info->publish_interval;

    self->_tick_feed_interval = 1000;

    bc_os_task_init(&self->task, task_worker, self );

    bc_log_info("_task_spawn_worker: spawned instance for bus %d, address 0x%02X",
                (uint8_t) task_info->i2c_channel, task_info->device_address);

    task_info->worker = self;
}

static void _task_worker_terminate(task_info_t *task_info)
{

    task_worker_t *self = NULL;

    bc_log_info("_task_worker_terminate: terminating instance for bus %d, address 0x%02X ",
                (uint8_t) task_info->i2c_channel, task_info->device_address);

    task_lock(task_info);
    if (task_info->worker == NULL)
    {

        task_unlock(task_info);

        bc_log_info("_task_worker_terminate: terminated instance for bus %d, address 0x%02X ",
                    (uint8_t) task_info->i2c_channel, task_info->device_address);

        return;
    }
    self = task_info->worker;

    self->_quit = true;
    task_unlock(task_info);

    bc_os_semaphore_put(&self->semaphore);

    bc_os_task_destroy(&self->task);

    task_lock(task_info);

    bc_os_semaphore_destroy(&self->semaphore);

    free(self);

    task_info->worker = NULL;

    task_unlock(task_info);

    bc_log_info("_task_worker_terminate: terminated instance for bus %d, address 0x%02X ",
                (uint8_t) task_info->i2c_channel, task_info->device_address);
}