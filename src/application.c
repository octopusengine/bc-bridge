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
size_t task_info_list_length = sizeof(task_info_list) / sizeof(task_info_t);

static void _application_wait_start_string(void);
static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel);
static void _application_bc_talk_callback(bc_talk_event_t *event);

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

    task_init(&bridge, &task_info_list, task_info_list_length );

//


    char testc[] = "[\"led/-/set\",{\"state\":\"2-dott\"}]";
    bc_talk_parse(testc, sizeof(testc), _application_bc_talk_callback);
    char testd[] = "[\"led/-/get\",{\"state\":null}]";
    bc_talk_parse(testd, sizeof(testd), _application_bc_talk_callback);
    exit(0);
//    char test[] = "[\"$config/sensors/thermometer/i2c0-48/update\", {\"publish-interval\": 500, \"aaa\": true}]";
//    bc_talk_parse(test, sizeof(test), _application_bc_talk_callback);
//    char testb[] = "[\"$config/sensors/lux-meter/i2c0-44/update\", {\"publish-interval\": 100}]";
//    bc_talk_parse(testb, sizeof(testb), _application_bc_talk_callback);


}

void application_loop(bool *quit)
{
    char *line = NULL;
    size_t length;

    for(;;)
    {
        if (getline(&line, &length, stdin) != -1)
        {
            bc_talk_parse(line, length, _application_bc_talk_callback);
        }
    }
}

static void _application_wait_start_string(void)
{
    //TODO predelat
    char *line = NULL;
    size_t size;
    while (true)
    {
        if (getline(&line, &size, stdin) != -1)
        {
            if (strcmp(line, "[\"$config/clown.talk/-/create\", {}]\n")==0)
            {
                printf("[\"$config/clown.talk/-\", {\"ack\":false, \"device\":\"bridge\", \"capabilities\":1,  \"firmware-datetime\":\"%s\"]\n", VERSION);
                return;
            }
        }
    }
}

static void _application_bc_talk_callback(bc_talk_event_t *event)
{
    bc_log_info("_application_bc_talk_callback: i2c_channel=%d device_address=%02X", event->i2c_channel, event->device_address);

    task_info_t task_info;
    bc_bridge_i2c_channel_t channel = event->i2c_channel == 0 ? BC_BRIDGE_I2C_CHANNEL_0 : BC_BRIDGE_I2C_CHANNEL_1;

    if (event->device_address!=0)
    {
        int i;
        bool find=false;
        for (i = 0; i < task_info_list_length; ++i)
        {
            if ( (task_info_list[i].i2c_channel == channel) && (task_info_list[i].device_address == event->device_address) )
            {
                task_info = task_info_list[i];
                find = true;
            }
        }
        if (!find)
        {
            return;
        }
    }


    switch (event->operation)
    {
        case BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL:
        {
            bc_log_info("_application_bc_talk_callback: UPDATE_PUBLISH_INTERVAL %d", (bc_tick_t) event->value);
            task_set_interval(&task_info, (bc_tick_t) event->value );
            break;
        }
        case BC_TALK_OPERATION_LED_SET:
        {
            bc_bridge_led_set(&bridge, event->value == 1 ? BC_BRIDGE_LED_ON : BC_BRIDGE_LED_OFF);
            break;
        }
        case BC_TALK_OPERATION_LED_GET:
        {
            bc_bridge_led_t value;
            bc_bridge_led_get(&bridge, &value);
            bc_talk_publish_led_state(value);
            break;
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

