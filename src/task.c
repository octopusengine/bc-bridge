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
#include "bc_i2c_pca9535.h"
#include "bc_tag_temperature.h"
#include "bc_tag_lux_meter.h"

#include "task.h"

void task_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        if (task_info_list[i].device_address == 0x00) //bridge led
        {
            task_led_spawn(bridge, &task_info_list[i]);
        }
        else if (bc_bridge_i2c_ping(bridge, task_info_list[i].i2c_channel, task_info_list[i].device_address))
        {

            switch (task_info_list[i].type)
            {
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
            task_led_set_interval((task_led_t *)task_info->task, interval );
            return true;
        }
        case TAG_THERMOMETHER:
        {
            task_thermometer_set_interval((task_thermometer_t *)task_info->task, interval );
            return true;
        }
        case TAG_LUX_METER:
        {
            task_lux_meter_set_interval((task_lux_meter_t*)task_info->task, interval );
            return true;
        }
        case TAG_BAROMETER:
        {
            task_barometer_set_interval((task_barometer_t*)task_info->task, interval );
            return true;
        }
        case TAG_HUMIDITY:
        {
            task_humidity_set_interval((task_humidity_t*)task_info->task, interval );
            return true;
        }
        case MODULE_CO2:
        {
            task_co2_set_interval((task_co2_t*)task_info->task, interval );
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
            task_led_get_interval((task_led_t *)task_info->task, interval );
            return true;
        }
        case TAG_THERMOMETHER:
        {
            task_thermometer_get_interval((task_thermometer_t *)task_info->task, interval );
            return true;
        }
        case TAG_LUX_METER:
        {
            task_lux_meter_get_interval((task_lux_meter_t*)task_info->task, interval );
            return true;
        }
        case TAG_BAROMETER:
        {
            task_barometer_get_interval((task_barometer_t*)task_info->task, interval );
            return true;
        }
        case TAG_HUMIDITY:
        {
            task_humidity_get_interval((task_humidity_t*)task_info->task, interval );
            return true;
        }
        case MODULE_CO2:
        {
            task_co2_get_interval((task_co2_t*)task_info->task, interval );
            return true;
        }
        default:
            bc_log_error("task_get_interval: unknown task_info->type=%d", task_info->type);
            return false;
    }
    return false;
}


