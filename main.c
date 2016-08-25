#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bc/bridge.h"
#include "jsmn.h"
#include <bc/tick.h>
#include <bc/i2c/sys.h>
#include <bc/i2c/app.h>
#include <bc/i2c/tca9534a.h>
#include <bc/tag/humidity.h>
#include <bc/tag/temperature.h>
#include <bc/module/co2.h>
#include <bc/module/relay.h>

int main (int argc, char *argv[])
{

    fprintf(stderr,"Bridge build %s \n", VERSION );
    
	bc_bridge_device_info_t devices[6];
	uint8_t length;
	bc_bridge_t bridge;

	bc_bridge_scan(devices, &length);

	fprintf(stderr,"found devices %d \n", length );

	if (!bc_bridge_open(&bridge, &devices[0]))
	{
		return EXIT_FAILURE;
	}

	sleep(60);


    return EXIT_SUCCESS;
}
