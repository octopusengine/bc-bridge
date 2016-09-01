#include "application.h"
#include "bc_log.h"
#include <signal.h>
#include <argp.h>

void  INThandler(int sig)
{
    char  c;

    signal(sig, SIG_IGN);
    printf("Do you really want to quit? [y/n] ");
    c = getchar();
    if (c == 'y' || c == 'Y')
        exit(0);
    else
        signal(SIGINT, INThandler);
    getchar(); // Get new line character
}

static char doc[] = "Argp example #2 -- a pretty minimal program using argp";

static struct argp_option options[] = {
        {"no-wait-start-string",  'n', 0, 0,  "no wait start string" },
        {"debug",  'd', "level",  0, "dump|debug|info|warning|error|fatal" },
        { 0 }
};

struct arguments
{
    bool no_wait_start_string;
    bc_log_level_t log_level;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    int a;

    switch (key)
    {
        case 'n':
            arguments->no_wait_start_string = true;
            break;
        case 'd':
            if (strcmp(arg, "dump")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_DUMP;
            }
            else if (strcmp(arg, "debug")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_DEBUG;
            }
            else if (strcmp(arg, "info")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_INFO;
            }
            else if (strcmp(arg, "warning")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_WARNING;
            }
            else if (strcmp(arg, "error")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_ERROR;
            }
            else if (strcmp(arg, "fatal")==0)
            {
                arguments->log_level = BC_LOG_LEVEL_FATAL;
            }
            else
            {
                return ARGP_ERR_UNKNOWN;
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

void process_command_line(int argc, char **argv)
{
    struct arguments arguments;

    arguments.no_wait_start_string = false;
    arguments.log_level = BC_LOG_LEVEL_DUMP;

    signal(SIGINT, INThandler);

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
}

int main(int argc, char **argv)
{
    bool quit;

    process_command_line(argc, argv);

    // TODO Diskutovat smysluplnost - myslim ze by v application_loop stacilo cekat blokujicim zpusobem na stdin ?
    // TODO Tzn CTRL+C apod neoverridovat...
    signal(SIGINT, INThandler);

    application_init();

    quit = false;

    while (!quit)
    {
        application_loop(&quit);
    }
}

#if 0

    if (!bc_bridge_open(&bridge, &devices[0]))
    {
        bc_log_fatal("main: call failed: bc_bridge_open");
    }

    bc_tag_interface_t tag_i2c0_interface = {
            .channel = BC_BRIDGE_I2C_CHANNEL_0,
            .bridge = &bridge
    };

    bc_module_co2_t module_co2;

    if (!bc_module_co2_init(&module_co2, &tag_i2c0_interface))
    {
        bc_log_fatal("main: call failed: bc_module_co2_init");
    }

    uint8_t address;
    uint8_t buffer[1];
    bc_bridge_i2c_transfer_t transfer;
    transfer.channel = BC_BRIDGE_I2C_CHANNEL_0;
    transfer.buffer = buffer;
    transfer.length = 1;
    for (address = 1; address < 128; address++)
    {
        transfer.device_address = address;
        if (bc_bridge_i2c_write(&bridge, &transfer))
        {
            printf("address: %hhx %d \n", address, address);
        }
    }


    tags_temperature_init(&tag_temperature, &tag_i2c0_interface, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT);
    tags_humidity_init(&tag_humidity, &tag_i2c0_interface);
    tags_lux_meter_init(&tag_lux_meter, &tag_i2c0_interface);
    tags_barometer_init(&tag_barometer, &tag_i2c0_interface);

    bc_module_relay_init(&module_relay, &tag_i2c0_interface);
    bc_module_relay_mode_t relay_state = BC_MODULE_RELAY_MODE_NO;

    fseek(stdin,0,SEEK_END); // vyprazdneni stdin bufferu TODO: otazka zda je to dobre
    while(1){
        char *line = NULL;
        size_t size;
        if (getline(&line, &size, stdin) == -1) {
            printf("No line\n");
        } else {
            printf("%s\n", line);
        }

        bc_log_debug("---------");
        bc_module_co2_task(&module_co2);
        if (!module_co2._co2_concentration_unknown)
        {
            printf(" co2 %d \n", module_co2._co2_concentration );
        }

        tags_temperature_task(&tag_temperature, &data);
        if (!data.null)
        {
            printf("[\"thermometer\", {\"0/temperature\": [%0.2f, \"\\u2103\"]}\n", data.value );
        }

        tags_humidity_task(&tag_humidity, &data);
        if (!data.null)
        {
            printf("[\"humidity-sensor\", {\"0/relative-humidity\":[%0.2f, \"%%\"]}]\n", data.value );
        }

        tags_lux_meter_task(&tag_temperature, &data);
        if (!data.null)
        {
            printf("[\"lux-meter\", {\"0/illuminance\":[%d, \"lux\"]}\n", (uint16_t)data.value );
        }

        tags_barometer_task(&tag_barometer, &absolute_pressure, &altitude);
        if (!absolute_pressure.null)
        {
            printf("[\"barometer\", {\"0/pressure\": [%0.2f, \"kPa\"]}\n", absolute_pressure.value );
        }
        if (!altitude.null)
        {
            printf("[\"barometer\", {\"0/altitude\": [%0.1f, \"m\"]}\n", altitude.value );
        }

        relay_state = relay_state == BC_MODULE_RELAY_MODE_NO ? BC_MODULE_RELAY_MODE_NC : BC_MODULE_RELAY_MODE_NO;
        bc_module_relay_set_mode(&module_relay, relay_state);

        sleep(1);
        printf("------------------------------------\n");
    }

    return EXIT_SUCCESS;
}

#endif
