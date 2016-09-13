#include "application.h"
#include "bc_talk.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"
#include "task_led.h"
#include "bc_i2c_pca9535.h"
#include "bc_tag_temperature.h"
#include "bc_tag_lux_meter.h"
#include "bc_tag_barometer.h"
#include "bc_tag_humidity.h"

#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"
#include <jsmn.h>

bc_bridge_t bridge;

task_info_t task_info_list[] = {
        {TASK_CLASS_BRIDGE, TASK_TYPE_LED, BC_BRIDGE_I2C_CHANNEL_0, 0x00, NULL, false},

        {TASK_CLASS_SENSOR, TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_LUX_METER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_LUX_METER_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_LUX_METER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_LUX_METER_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_BAROMETER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_BAROMETER_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_BAROMETER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_BAROMETER_ADDRESS_DEFAULT, NULL, false},
        {TASK_CLASS_SENSOR, TAG_HUMIDITY, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_HUMIDITY_DEVICE_ADDRESS, NULL, false},
        {TASK_CLASS_SENSOR, TAG_HUMIDITY, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_HUMIDITY_DEVICE_ADDRESS, NULL, false},

        {TASK_CLASS_SENSOR, MODULE_CO2, BC_BRIDGE_I2C_CHANNEL_0, 0x38, NULL, false},
        {TASK_CLASS_ACTUATOR, MODULE_RELAY, BC_BRIDGE_I2C_CHANNEL_0, 0x3B, NULL, false}

};
size_t task_info_list_length = sizeof(task_info_list) / sizeof(task_info_t);


static void _application_wait_start_string(void);
static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel);
static void _application_bc_talk_callback(bc_talk_event_t *event);

void application_init(application_parameters_t *parameters)
{
    bc_talk_init();
    bc_log_init(parameters->log_level);
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

    if (!parameters->furious_mode)
    {
        _application_wait_start_string();
    }

//    _application_i2c_scan(&bridge, BC_BRIDGE_I2C_CHANNEL_0);

    task_init(&bridge, task_info_list, task_info_list_length );

//


//    char testc[] = "[\"led/-/set\",{\"state\":\"3-dot\"}]";
//    bc_talk_parse(testc, sizeof(testc), _application_bc_talk_callback);
//    char testd[] = "[\"led/-/get\",{\"state\":null}]";
//    bc_talk_parse(testd, sizeof(testd), _application_bc_talk_callback);

//    char test[] = "[\"$config/devices/thermometer/i2c0-48/update\", {\"publish-interval\": 500, \"aaa\": true}]";
//    bc_talk_parse(test, sizeof(test), _application_bc_talk_callback);
//
//    char testb[] = "[\"relay/i2c0-3b/set\",{\"state\":false}]";
//    bc_talk_parse(testb, sizeof(testb), _application_bc_talk_callback);
//
//    char teste[] = "[\"$config/devices/led/-/update\",{\"publish-interval\":5000}]";
//    bc_talk_parse(teste, sizeof(teste), _application_bc_talk_callback);


//    char testq[] = "[\"$config/devices/-/-/list\",{}]";
//    bc_talk_parse(testq, sizeof(testq), _application_bc_talk_callback);
//
//    exit(0);
}

void application_loop(bool *quit)
{
    char *line;
    size_t length;

    for(;;)
    {
        line = NULL;

        if (getline(&line, &length, stdin) != -1)
        {
            bc_talk_parse(line, length, _application_bc_talk_callback);
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
            if (bc_talk_parse_start(line, size))
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

    int i;

    if (event->operation!=BC_TALK_OPERATION_CONFIG_LIST)
    {
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
            bc_log_error("_application_bc_talk_callback: tag or module not found");
            return;
        }

        if (!task_info.enabled)
        {
            bc_log_error("_application_bc_talk_callback: tag or module not enabled");
            return;
        }

    }

    char topic[64];

    switch (event->operation)
    {
        case BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL:
        {
            bc_log_info("_application_bc_talk_callback: UPDATE_PUBLISH_INTERVAL %d", (bc_tick_t) event->value);
            task_set_interval(&task_info, (bc_tick_t) event->value );
            //break; //necham propadnout at se vypise potvrzeni o nastaveni
        }
        case BC_TALK_OPERATION_CONFIG_READ:
        {
            bc_tick_t publish_interval;
            if (task_get_interval(&task_info, &publish_interval)){
                char tmp[64];
                bc_talk_make_topic( task_info.i2c_channel, task_info.device_address, tmp, sizeof(tmp) );
                snprintf(topic, sizeof(topic), "$config/devices/%s", tmp );
                bc_talk_publish_begin(topic);
                bc_talk_publish_add_value("publish-interval", "%d", publish_interval );
                bc_talk_publish_end();
            }
            break;
        }
        case BC_TALK_OPERATION_RELAY_SET :
        {
            if ( event->value == -1)
            {
                bc_log_info("_application_bc_talk_callback: BC_TALK_OPERATION_RELAY_SET bad value");
                return;
            }
            task_relay_set_mode((task_relay_t *)task_info.task, event->value==1 ? TASK_RELAY_MODE_TRUE : TASK_RELAY_MODE_FALSE);
            break;
        }
        case BC_TALK_OPERATION_RELAY_GET:
        {
            task_relay_mode_t relay_mode;
            task_relay_get_mode((task_relay_t *)task_info.task, &relay_mode);

            bc_talk_publish_relay((int)relay_mode, task_info.device_address);
            break;
        }
        case BC_TALK_OPERATION_LED_SET:
        {
            if ( event->value == -1)
            {
                bc_log_info("_application_bc_talk_callback: BC_TALK_OPERATION_LED_SET bad value");
                return;
            }
            task_led_set_state((task_led_t *)task_info.task, (task_led_state_t)event->value);
            //break; //necham propadnout at se vypise potvrzeni o nastaveni
        }
        case BC_TALK_OPERATION_LED_GET:
        {
            task_led_state_t led_state;
            task_led_get_state((task_led_t *)task_info.task, &led_state);
            bc_talk_publish_led_state((int)led_state);
            break;
        }
        case BC_TALK_OPERATION_CONFIG_LIST:
        {
            char sensors[1024];
            char actuators[1024];

            bool sensors_empty = true;
            bool actuators_empty = true;

            memset(sensors, 0, sizeof(sensors));
            memset(actuators, 0, sizeof(actuators));


            for (i = 0; i < task_info_list_length; ++i)
            {
                if (task_info_list[i].enabled)
                {
                    if (task_info_list[i].class == TASK_CLASS_SENSOR)
                    {
                        bc_talk_make_topic(task_info_list[i].i2c_channel, task_info_list[i].device_address, topic, sizeof(topic));
                        if (sensors_empty)
                        {
                            strcat(sensors, "\"");
                            sensors_empty = false;
                        }
                        else
                        {
                            strcat(sensors, "\", \"");
                        }
                        strcat(sensors, topic);
                    }
                    else if (task_info_list[i].class == TASK_CLASS_ACTUATOR)
                    {
                        bc_talk_make_topic(task_info_list[i].i2c_channel, task_info_list[i].device_address, topic, sizeof(topic));
                        if (actuators_empty)
                        {
                            strcat(actuators, "\"");
                            actuators_empty = false;
                        }
                        else
                        {
                            strcat(actuators, "\", \"");
                        }
                        strcat(actuators, topic);
                    }

                }


            }

            if (!sensors_empty)
            {
                strcat(sensors, "\"");
            }

            if (!actuators_empty)
            {
                strcat(actuators, "\"");
            }

            bc_talk_publish_begin("$config/devices/-/-");
            bc_talk_publish_add_value("sensors", "[%s]", sensors);
            bc_talk_publish_add_value("actuators", "[%s]", actuators);
            bc_talk_publish_end();
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
