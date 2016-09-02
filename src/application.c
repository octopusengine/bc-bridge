#include "application.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "bc_tag.h"
#include "bc_bridge.h"

bc_bridge_t bridge;

task_thermometer_t *thermometer_1=NULL;

static bool _application_is_device_exists(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address);

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

    if(_application_is_device_exists(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x48))
    {
        thermometer_1 = task_thermometer_spawn(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x48);
    }

}

void application_loop(bool *quit)
{
    bc_log_debug("---");

    if(thermometer_1!=NULL)
    {
        bc_os_semaphore_put(&thermometer_1->semaphore);
    }

    bc_os_task_sleep(1000);
}

static bool _application_is_device_exists(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address)
{
    bc_tag_transfer_t transfer;
    uint8_t buffer[1];

    bc_tag_transfer_init(&transfer);

    transfer.device_address = device_address;
    transfer.buffer = buffer;
    transfer.address = 0x00;
    transfer.length = 1;

    buffer[0] = 0x00;

#ifdef BRIDGE
    transfer.channel = i2c_channel;
    if (!bc_bridge_i2c_write(bridge, &transfer))
    {
        return false;
    }
#else
    bool communication_fault;

    if (!self->_interface->write(&transfer, &communication_fault))
    {
        return false;
    }
#endif

    return true;
}