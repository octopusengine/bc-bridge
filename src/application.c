#include "application.h"
#include "bc_talk.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "bc_i2c_pca9535.h"
#include "bc_tag_temperature.h"
#include "bc_tag_lux_meter.h"

#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"
#include <jsmn.h>

bc_bridge_t bridge;

task_info_t task_info_list[] = {
        {SENSOR, TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT, NULL, false},
        {SENSOR, TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT, NULL, false},
        {SENSOR, TAG_LUX_METER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_LUX_METER_ADDRESS_DEFAULT, NULL, false},
        {SENSOR, TAG_LUX_METER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_LUX_METER_ADDRESS_DEFAULT, NULL, false},
        {SENSOR, MODULE_CO2, BC_BRIDGE_I2C_CHANNEL_0, 0x38, NULL, false},
        {ACTUATOR, MODULE_RELAY, BC_BRIDGE_I2C_CHANNEL_0, 0x3B, NULL, false},
};

static void _application_wait_start_string(void);
static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel);

void application_init(bool wait_start_string, bc_log_level_t log_level)
{
    bc_talk_init();
    bc_log_init(log_level);
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

    if (wait_start_string == true)
    {
        _application_wait_start_string();
    }

//    _application_i2c_scan(&bridge, BC_BRIDGE_I2C_CHANNEL_0);

    task_init(&bridge, &task_info_list, sizeof(task_info_list) / sizeof(task_info_t) );
}

void application_loop(bool *quit)
{
    char *line = NULL;
    size_t length;

    for(;;)
    {
        if (getline(&line, &length, stdin) != -1)
        {
            bc_talk_parse(&line, length);
        }
    }
}

static void _application_wait_start_string(void)
{
    char *line = NULL;
    size_t size;
    while (true)
    {
        if (getline(&line, &size, stdin) != -1)
        {
            if (strcmp(line, "[\"$config/clown-talk/create\", {}]\n")==0)
            {
                printf("[\"$config/clown-talk\", {\"ack\":false, \"device\":\"bridge\", \"capabilities\":1,  \"firmware-datetime\":\"%s\"]\n", VERSION);
                return;
            }
        }
    }
}

static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel)
{
    uint8_t address;
    for (address = 1; address < 128; address++)
    {
        if (bc_bridge_i2c_ping(bridge, i2c_channel, address))
        {
            printf("address: %hhx %d \n", address, address);
        }
    }
    exit(1);
}

static void _application_quad_test(bc_bridge_t *bridge)
{
    bc_i2c_pca9535_t pca;
    bc_i2c_interface_t interface = {
            .bridge = bridge,
            .channel = BC_BRIDGE_I2C_CHANNEL_0
    };

    bc_ic2_pca9535_init(&pca, &interface, 0x24);
    bc_ic2_pca9535_set_modes(&pca, BC_I2C_pca9535_PORT1, BC_I2C_pca9535_ALL_OUTPUT);
//    exit(1);

    while (true)
    {
        bc_ic2_pca9535_write_pins(&pca, BC_I2C_pca9535_PORT1, 0x0f);
        bc_os_task_sleep(100);
        bc_ic2_pca9535_write_pins(&pca, BC_I2C_pca9535_PORT1, 0x00);
        bc_os_task_sleep(100);
    }
}