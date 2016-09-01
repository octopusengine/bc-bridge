#include "application.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "bc_bridge.h"

bc_bridge_t bridge;

task_thermometer_t *thermometer_1;

void application_init(void)
{
    bc_log_init(BC_LOG_LEVEL_DUMP);

    bc_tick_init();

    bc_log_info("application_init: build %s", VERSION);

    // TODO predelat na dynamicke pole
    bc_bridge_device_info_t devices[6];

    uint8_t device_count;

    if (!bc_bridge_scan(devices, &device_count))
    {
        bc_log_fatal("application_init: call failed: bc_bridge_scan");
    }

    bc_log_info("application_init: number of found devices: %d", device_count);

    if (device_count == 0)
    {
        bc_log_fatal("application_init: no devices have been found");
    }

    if (!bc_bridge_open(&bridge, &devices[0]))
    {
        bc_log_fatal("application_init: call failed: bc_bridge_open");
    }

    thermometer_1 = task_thermometer_spawn(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x48);
}

void application_loop(bool *quit)
{
    bc_log_debug("---");

    bc_os_semaphore_put(&thermometer_1->semaphore);

    bc_os_task_sleep(1000);
}
