#include "application.h"
#include "bc_talk.h"
#include "bc_log.h"
#include "task_thermometer.h"
#include "task_lux_meter.h"
#include "task_relay.h"
#include "task_co2.h"

#include "bc_tag.h"
#include "bc_bridge.h"
#include <jsmn.h>

bc_bridge_t bridge;


task_thermometer_t *thermometer_0=NULL;
task_lux_meter_t *lux_meter_0=NULL;
task_relay_t *relay=NULL;
task_co2_t *co2=NULL;

static void _application_wait_start_string(void);
static bool _application_is_device_exists(bc_bridge_t *bridge, bc_bridge_i2c_channel_t i2c_channel, uint8_t device_address);
static bool _application_jsoneq(const char *json, jsmntok_t *tok, const char *s);

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

    if(bc_bridge_i2c_ping(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x48))
    {
        task_thermometer_parameters_t parameters;

        parameters.bridge = &bridge;
        parameters.i2c_channel = BC_BRIDGE_I2C_CHANNEL_0;
        parameters.device_address = 0x48;

        thermometer_0 = task_thermometer_spawn(&parameters);
    }

    if(bc_bridge_i2c_ping(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x44))
    {
        lux_meter_0_44 = task_lux_meter_spawn(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x44);
    }
    if(bc_bridge_i2c_ping(&bridge, BC_BRIDGE_I2C_CHANNEL_1, 0x44))
    {
        lux_meter_1_44 = task_lux_meter_spawn(&bridge, BC_BRIDGE_I2C_CHANNEL_1, 0x44);
    }

    // moduly
    if(bc_bridge_i2c_ping(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x3B))
    {
        relay = task_relay_spawn(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x3B);
    }

    if(bc_bridge_i2c_ping(&bridge, BC_BRIDGE_I2C_CHANNEL_0, 0x38))
    {
        co2 = task_co2_spawn(&bridge);
    }


}

void application_loop(bool *quit)
{
    char *line = NULL;
    size_t size;
    jsmn_parser parser;
    jsmntok_t tokens[10];
    int r;

    for(;;)
    {
        if (getline(&line, &size, stdin) == -1)
        {
            printf("No line\n");
        } else
        {

            jsmn_init(&parser);
            r = jsmn_parse(&parser, line, size, tokens, 10 );
            if (r < 0) {
                bc_log_error("application_loop: talk parser: Failed to parse JSON: %d", r);
                return;
            }

            if (r < 1 || tokens[0].type != JSMN_ARRAY) {
                bc_log_error("application_loop: talk parser: Array expected");
                return;
            }

            if (_application_jsoneq(line, &tokens[1], "$config/sensors/thermometer/update") && (r==5))
            {
                if (_application_jsoneq(line, &tokens[3], "publish-interval"))
                {
                    int number = strtol(line+tokens[4].start, NULL, 10);
                    printf("number %d \n", number);
                    bc_log_info("application_loop: thermometer new publish-interval %d", number);
                    if(thermometer_0) //TODO v tuto chvily by nemel byt problem, bud je inicializovany nebo neni
                    {
                        task_thermometer_set_interval(thermometer_0, (bc_tick_t)number);
                    }
                }
            }
            else if(_application_jsoneq(line, &tokens[1], "relay/set") && (r==5))
            {
                if (relay && _application_jsoneq(line, &tokens[3], "0/state"))
                {
                    if ( strncmp(line + tokens[4].start, "true", tokens[4].end - tokens[4].start) == 0)
                    {
                        task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NO);
                    }
                    else if ( strncmp(line + tokens[4].start, "false", tokens[4].end - tokens[4].start) == 0)
                    {
                        task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NC);
                    }

                }
            }

            //task_thermometer_set_interval(thermometer_0, 500);

//            printf("%s\n", line);
        }

    }

//    bc_log_debug("---");
//
//    if(thermometer_0!=NULL)
//    {
//        bc_os_semaphore_put(&thermometer_0->semaphore);
//    }

//    bc_os_task_sleep(1000);
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

static bool _application_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0)
    {
        return true;
    }
    return false;
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
