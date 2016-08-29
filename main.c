#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bc/bridge.h"
#include <jsmn.h>
#include "tags.h"


tags_data_t data;
tags_data_t absolute_pressure, altitude;

bc_tag_humidity_t tag_humidity;
bc_tag_temperature_t tag_temperature;
bc_tag_lux_meter_t tag_lux_meter;
bc_tag_barometer_t tag_barometer;

bc_module_relay_t module_relay;

int main (int argc, char *argv[])
{

    fprintf(stderr,"Bridge build %s \n", VERSION );
    
    bc_bridge_device_info_t devices[6];//TODO predelat na dinamicke pole

    uint8_t length;
    bc_bridge_t bridge;

    bc_bridge_scan(devices, &length);

    fprintf(stderr,"found devices %d \n", length );

    if (length==0)
    {
        fprintf(stderr,"no found any devices, exit \n");
        return EXIT_SUCCESS;
    }

    if (!bc_bridge_open(&bridge, &devices[0]))
    {
        return EXIT_FAILURE;
    }

    bc_tag_interface_t tag_i2c0_interface = {
            .channel = BC_BRIDGE_I2C_CHANNEL_0,
            .bridge = &bridge
    };

    tags_humidity_init(&tag_humidity, &tag_i2c0_interface);
    tags_temperature_init(&tag_temperature, &tag_i2c0_interface, BC_TAG_TEMPERATURE_ADDRESS_DEFAULT);
    tags_lux_meter_init(&tag_lux_meter, &tag_i2c0_interface);
    tags_barometer_init(&tag_barometer, &tag_i2c0_interface);

    //bc_module_relay_init(&module_relay, &tag_i2c0_interface);
    //bc_module_relay_mode_t relay_state = BC_MODULE_RELAY_MODE_NO;

    while(1){

        //relay_state = relay_state == BC_MODULE_RELAY_MODE_NO ? BC_MODULE_RELAY_MODE_NC : BC_MODULE_RELAY_MODE_NO;

        tags_humidity_task(&tag_humidity, &data);
        if (!data.null)
        {
            printf("[\"humidity-sensor\", {\"0/relative-humidity\":[%0.2f, \"%%\"]}]\n", data.value );
        }

        tags_temperature_task(&tag_temperature, &data);
        if (!data.null)
        {
            printf("[\"thermometer\", {\"0/temperature\": [%0.2f, \"\\u2103\"]}\n", data.value );
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

        //bc_module_relay_set_mode(&module_relay, relay_state);

        sleep(1);
        //printf("------------------------------------\n");
    }


    return EXIT_SUCCESS;
}
