#include "application.h"
#include "bc_bridge.h"
#include "bc_talk.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "task_led.h"
#include "task_barometer.h"
#include "task_humidity.h"
#include "task.h"

void task_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        switch (task_info_list[i].type)
        {
            case TASK_TYPE_LED:
            {
                task_led_spawn(bridge, &task_info_list[i]);
                break;
            }
            case TAG_THERMOMETHER:
            {
                task_thermometer_spawn(bridge, &task_info_list[i]);
                break;
            }
            case TAG_LUX_METER:
            {
                task_lux_meter_spawn(bridge, &task_info_list[i]);
                break;
            }
            case TAG_BAROMETER:
            {
                task_barometer_spawn(bridge, &task_info_list[i]);
                break;
            }
            case TAG_HUMIDITY:
            {
                task_humidity_spawn(bridge, &task_info_list[i]);
                break;
            }
            case MODULE_RELAY:
            {
                task_relay_spawn(bridge, &task_info_list[i]);
                break;
            }
            case MODULE_CO2:
            {
                task_co2_spawn(bridge, &task_info_list[i]);
                break;
            }
            default:
                break;
        }

    }
}

void task_destroy(task_info_t *task_info_list, size_t length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        switch (task_info_list[i].type)
        {
            case TASK_TYPE_LED:
            {
                task_led_terminate((task_led_t *) task_info_list[i].task);
                break;
            }
            case TAG_THERMOMETHER:
            {
                task_thermometer_terminate((task_thermometer_t *) task_info_list[i].task);
                break;
            }
            case TAG_LUX_METER:
            {
                task_lux_meter_terminate((task_lux_meter_t *) task_info_list[i].task);
                break;
            }
            case TAG_BAROMETER:
            {
                task_barometer_terminate((task_barometer_t *) task_info_list[i].task);
                break;
            }
            case TAG_HUMIDITY:
            {
                task_humidity_terminate((task_humidity_t *) task_info_list[i].task);
                break;
            }
            case MODULE_RELAY:
            {
                task_relay_terminate((task_relay_t *) task_info_list[i].task);
                break;
            }
            case MODULE_CO2:
            {
                task_co2_terminate((task_co2_t *) task_info_list[i].task);
                break;
            }
            default:
                break;
        }
        task_info_list[i].task = NULL;
        task_info_list[i].enabled = false;
    }
}

bool task_set_interval(task_info_t *task_info, bc_tick_t interval)
{
    if (!task_info->enabled)
    {
        return false;
    }
    switch (task_info->type)
    {
        case TASK_TYPE_LED:
        {
            task_led_set_interval((task_led_t *) task_info->task, interval);
            return true;
        }
        case TAG_THERMOMETHER:
        {
            task_thermometer_set_interval((task_thermometer_t *) task_info->task, interval);
            return true;
        }
        case TAG_LUX_METER:
        {
            task_lux_meter_set_interval((task_lux_meter_t *) task_info->task, interval);
            return true;
        }
        case TAG_BAROMETER:
        {
            task_barometer_set_interval((task_barometer_t *) task_info->task, interval);
            return true;
        }
        case TAG_HUMIDITY:
        {
            task_humidity_set_interval((task_humidity_t *) task_info->task, interval);
            return true;
        }
        case MODULE_CO2:
        {
            task_co2_set_interval((task_co2_t *) task_info->task, interval);
            return true;
        }
        default:
            bc_log_error("task_set_interval: unknown task_info->type=%d", task_info->type);
            return false;
    }
    return false;
}

bool task_get_interval(task_info_t *task_info, bc_tick_t *interval)
{
    if (!task_info->enabled)
    {
        return false;
    }
    switch (task_info->type)
    {
        case TASK_TYPE_LED:
        {
            task_led_get_interval((task_led_t *) task_info->task, interval);
            return true;
        }
        case TAG_THERMOMETHER:
        {
            task_thermometer_get_interval((task_thermometer_t *) task_info->task, interval);
            return true;
        }
        case TAG_LUX_METER:
        {
            task_lux_meter_get_interval((task_lux_meter_t *) task_info->task, interval);
            return true;
        }
        case TAG_BAROMETER:
        {
            task_barometer_get_interval((task_barometer_t *) task_info->task, interval);
            return true;
        }
        case TAG_HUMIDITY:
        {
            task_humidity_get_interval((task_humidity_t *) task_info->task, interval);
            return true;
        }
        case MODULE_CO2:
        {
            task_co2_get_interval((task_co2_t *) task_info->task, interval);
            return true;
        }
        default:
            bc_log_error("task_get_interval: unknown task_info->type=%d", task_info->type);
            return false;
    }
    return false;
}

bool task_is_quit_request(task_info_t *task_info)
{
    switch (task_info->type)
    {
        case TASK_TYPE_LED:
        {
            return task_led_is_quit_request((task_led_t *) task_info->task);
        }
        case TAG_THERMOMETHER:
        {
            return task_thermometer_is_quit_request((task_thermometer_t *) task_info->task);
        }
        case TAG_LUX_METER:
        {
            return task_lux_meter_is_quit_request((task_lux_meter_t *) task_info->task);
        }
        case TAG_BAROMETER:
        {
            return task_barometer_is_quit_request((task_barometer_t *) task_info->task);
        }
        case TAG_HUMIDITY:
        {
            return task_humidity_is_quit_request((task_humidity_t *) task_info->task);
        }
        case MODULE_CO2:
        {
            return task_co2_is_quit_request((task_co2_t *) task_info->task);
        }
        case MODULE_RELAY:
        {
            return task_relay_is_quit_request((task_relay_t *) task_info->task);
        }
        default:
            bc_log_error("task_set_interval: unknown task_info->type=%d", task_info->type);
            return false;
    }
    return false;
}


