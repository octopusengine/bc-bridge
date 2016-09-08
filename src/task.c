#include "application.h"
#include "bc_bridge.h"
#include "bc_talk.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "bc_i2c_pca9535.h"
#include "bc_tag_temperature.h"
#include "bc_tag_lux_meter.h"

#include "task.h"

void task_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        task_info_t task_info = task_info_list[i];

        if (bc_bridge_i2c_ping(bridge, task_info.i2c_channel, task_info.device_address))
        {
            switch (task_info.type)
            {
                case TAG_THERMOMETHER:
                {
                    task_thermometer_spawn(bridge, &task_info);
                    break;
                }
                case TAG_LUX_METER:
                {
                    task_lux_meter_spawn(bridge, &task_info);
                    break;
                }
                case MODULE_RELAY:
                {
                    task_relay_spawn(bridge, &task_info);
                    break;
                }
                case MODULE_CO2:
                {
                    task_co2_spawn(bridge, &task_info);
                    break;
                }
            }
        }
    }
}
