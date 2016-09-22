#include "application.h"
#include "bc_talk.h"
#include "task_thermometer.h"
#include "task_relay.h"
#include "task_led.h"
#include "bc_tag_temperature.h"
#include "bc_tag_lux_meter.h"
#include "bc_tag_barometer.h"
#include "bc_tag_humidity.h"
#include "task.h"
#include "task_manager.h"

bc_bridge_t bridge;

task_info_t task_info_list[] =
    {
        { TASK_CLASS_ACTUATOR, TASK_TYPE_LED,              BC_BRIDGE_I2C_CHANNEL_0, 0x00 ,                                  NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT,     NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT,     NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_TEMPERATURE_ADDRESS_ALTERNATE,   NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_THERMOMETHER, BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_TEMPERATURE_ADDRESS_ALTERNATE,   NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_LUX_METER,    BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_LUX_METER_ADDRESS_DEFAULT,       NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_LUX_METER,    BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_LUX_METER_ADDRESS_DEFAULT,       NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_LUX_METER,    BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_LUX_METER_ADDRESS_ALTERNATE,     NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_LUX_METER,    BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_LUX_METER_ADDRESS_ALTERNATE,     NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_BAROMETER,    BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_BAROMETER_ADDRESS_DEFAULT,       NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_BAROMETER,    BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_BAROMETER_ADDRESS_DEFAULT,       NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_HUMIDITY,     BC_BRIDGE_I2C_CHANNEL_0, BC_TAG_HUMIDITY_DEVICE_ADDRESS_DEFAULT, NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_TAG_HUMIDITY,     BC_BRIDGE_I2C_CHANNEL_1, BC_TAG_HUMIDITY_DEVICE_ADDRESS_DEFAULT, NULL },
        { TASK_CLASS_ACTUATOR, TASK_TYPE_MODULE_RELAY,     BC_BRIDGE_I2C_CHANNEL_0, BC_MODULE_RELAY_ADDRESS_DEFAULT,        NULL },
        { TASK_CLASS_ACTUATOR, TASK_TYPE_MODULE_RELAY,     BC_BRIDGE_I2C_CHANNEL_0, BC_MODULE_RELAY_ADDRESS_ALTERNATE,      NULL },
        { TASK_CLASS_SENSOR,   TASK_TYPE_MODULE_CO2,       BC_BRIDGE_I2C_CHANNEL_0, 0x38,                                   NULL }
    };

size_t task_info_list_length = sizeof(task_info_list) / sizeof(task_info_t);

static void _application_wait_start_string(void);
//static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel);
static void _application_bc_talk_callback(bc_talk_event_t *event);

void application_init(application_parameters_t *parameters)
{
    bc_log_init(parameters->log_level);
    bc_tick_init();
    bc_talk_init(_application_bc_talk_callback);

    if (!parameters->furious_mode)
    {
        _application_wait_start_string();
    }

    memset(&bridge, 0, sizeof(bridge));
}

void application_loop(bool *quit)
{
    // TODO predelat na dynamicke pole
    bc_bridge_device_info_t devices[10];

    uint8_t device_count;
    bc_tick_t last_init=0;

    bc_log_info("application_loop: searching device");
    while (true)
    {

        if (!bc_bridge_scan(devices, &device_count))
        {
            bc_log_error("application_loop: call failed: bc_bridge_scan");

            return;
        }

        if (device_count > 0)
        {
            break;
        }

        bc_os_task_sleep(5000);

    }


    if (!bc_bridge_open(&bridge, &devices[0]))
    {
        bc_log_error("application_loop: call failed: bc_bridge_open");

        return;
    }

    task_manager_init(&bridge, task_info_list, task_info_list_length);
    last_init = bc_tick_get();

    while (bc_bridge_is_alive(&bridge))
    {

        if (bc_tick_get() > last_init+30000 )
        {
            task_manager_init(&bridge, task_info_list, task_info_list_length);
            last_init = bc_tick_get();
        }

        bc_os_task_sleep(1000);

    }

    bc_log_error("application_loop: device disconnect");

    task_manager_terminate(task_info_list, task_info_list_length);

    bc_bridge_close(&bridge);

    *quit = false;
}

static void _application_wait_start_string(void)
{
    while (true)
    {
        char *line;
        size_t size;

        line = NULL;

        if (getline(&line, &size, stdin) != -1)
        {
            if (bc_talk_parse_start(line, size))
            {
                printf(
                    "[\"$config/clown.talk/-\", {\"ack\":false, \"device\":\"bridge\", \"capabilities\":1,  \"firmware-datetime\":\"%s\", \"firmware-release\":\"%s\"}]\n",
                    FIRMWARE_DATETIME, FIRMWARE_RELEASE);

                return;
            }
        }
    }
}

static void _application_bc_talk_callback(bc_talk_event_t *event)
{
    task_info_t task_info;
    bc_bridge_i2c_channel_t channel;

    if (!bc_bridge_is_alive(&bridge))
    {
        return;
    }

    bc_log_info("_application_bc_talk_callback: i2c_channel=%d device_address=%02X", event->i2c_channel,
                event->device_address);

    channel = event->i2c_channel == 0 ? BC_BRIDGE_I2C_CHANNEL_0 : BC_BRIDGE_I2C_CHANNEL_1;

    if (event->operation != BC_TALK_OPERATION_CONFIG_LIST)
    {
        int i;

        for (i = 0; i < task_info_list_length; i++)
        {
            if ((task_info_list[i].i2c_channel == channel) &&
                (task_info_list[i].device_address == event->device_address))
            {
                task_info = task_info_list[i];

                break;
            }
        }

        if (i == task_info_list_length)
        {
            bc_log_error("_application_bc_talk_callback: tag or module not found");

            return;
        }

        if (!task_is_alive(&task_info))
        {
            bc_log_error("_application_bc_talk_callback: tag or module not enabled");

            return;
        }
    }

    switch (event->operation)
    {
        case BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL:
        {
            bc_log_debug("_application_bc_talk_callback: UPDATE_PUBLISH_INTERVAL %d", (bc_tick_t) event->value);

            task_set_interval(&task_info, (bc_tick_t) event->value);
        }
        case BC_TALK_OPERATION_CONFIG_READ:
        {
            bc_tick_t publish_interval;

            task_get_interval(&task_info, &publish_interval);

            char topic[64];
            char tmp[64];

            bc_talk_make_topic(task_info.i2c_channel, task_info.device_address, tmp, sizeof(tmp));

            snprintf(topic, sizeof(topic), "$config/devices/%s", tmp);

            bc_talk_publish_begin(topic);
            bc_talk_publish_add_value("publish-interval",
                                      publish_interval == BC_TALK_INT_VALUE_NULL ? "null" : "%d", publish_interval);
            bc_talk_publish_end();

            break;
        }
        case BC_TALK_OPERATION_RELAY_SET:
        {
            if (event->value == 1)
            {
                task_relay_set_state(&task_info, TASK_RELAY_STATE_TRUE);
            }
            else
            {
                task_relay_set_state(&task_info, TASK_RELAY_STATE_FALSE);
            }

            break;
        }
        case BC_TALK_OPERATION_RELAY_GET:
        {
            task_relay_state_t relay_state;

            task_relay_get_state(&task_info, &relay_state);

            bc_talk_publish_relay((int) relay_state, task_info.device_address);

            break;
        }
        case BC_TALK_OPERATION_LED_SET:
        {
            task_led_set_state(&task_info, (task_led_state_t) event->value);
        }
        case BC_TALK_OPERATION_LED_GET:
        {
            task_led_state_t led_state;

            task_led_get_state(&task_info, &led_state);

            bc_talk_publish_led_state((int) led_state);

            break;
        }
        case BC_TALK_OPERATION_CONFIG_LIST:
        {
            char sensors[1024];
            char actuators[1024];

            int i;

            strcpy(sensors, "");
            strcpy(actuators, "");

            for (i = 0; i < task_info_list_length; ++i)
            {
                if (task_is_alive(&task_info_list[i]))
                {
                    if (task_info_list[i].class == TASK_CLASS_SENSOR)
                    {
                        char topic[64];

                        bc_talk_make_topic(task_info_list[i].i2c_channel, task_info_list[i].device_address, topic,
                                           sizeof(topic));

                        if (strlen(sensors) == 0)
                        {
                            strcpy(sensors, "\"");
                        }
                        else
                        {
                            strcat(sensors, "\", \"");
                        }

                        strcat(sensors, topic);
                    }
                    else if (task_info_list[i].class == TASK_CLASS_ACTUATOR)
                    {
                        char topic[64];

                        bc_talk_make_topic(task_info_list[i].i2c_channel, task_info_list[i].device_address, topic,
                                           sizeof(topic));

                        if (strlen(actuators) == 0)
                        {
                            strcpy(actuators, "\"");
                        }
                        else
                        {
                            strcat(actuators, "\", \"");
                        }

                        strcat(actuators, topic);
                    }
                }
            }

            if (strlen(sensors) != 0)
            {
                strcat(sensors, "\"");
            }

            if (strlen(actuators) != 0)
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
//
//static void _application_i2c_scan(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel)
//{
//    uint8_t address;
//    for (address = 1; address < 128; address++)
//    {
//        if (bc_bridge_i2c_ping(bridge, i2c_channel, address))
//        {
//            printf("address: %hhx %d \n", address, address);
//        }
//    }
//
//}

//static void _application_quad_test(bc_bridge_t *bridge)
//{
//    bc_i2c_pca9535_t pca;
//    bc_i2c_interface_t interface = {
//        .bridge = bridge,
//        .channel = BC_BRIDGE_I2C_CHANNEL_0
//    };
//
//    bc_ic2_pca9535_init(&pca, &interface, 0x24);
//    bc_ic2_pca9535_set_modes(&pca, BC_I2C_pca9535_PORT1, BC_I2C_pca9535_ALL_OUTPUT);
////    exit(1);
//
//    while (true)
//    {
//        bc_ic2_pca9535_write_pins(&pca, BC_I2C_pca9535_PORT1, 0x0f);
//        bc_os_task_sleep(100);
//        bc_ic2_pca9535_write_pins(&pca, BC_I2C_pca9535_PORT1, 0x00);
//        bc_os_task_sleep(100);
//    }
//}
