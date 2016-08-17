#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bridge.h"
#include "jsmn.h"
#include "ft260.h"
#include <bc/tick.h>
#include <bc/i2c/sys.h>
#include <bc/i2c/app.h>
#include <bc/tag/humidity.h>
#include <bc/tag/temperature.h>

void blink(int cnt){
    for(int i=0;i<cnt;i++){
        ft260_led(i%2==0);
        sleep(1);
    }
    ft260_led(0);
}

bc_tag_humidity_t tag_humidity;
bc_tag_temperature_t tag_temperature;

static void demo_humidity_init(void)
{
	bc_tag_humidity_init(&tag_humidity, bc_i2c_sys_get_tag_interface());
}

static void demo_humidity_task(void)
{
	bc_tag_humidity_state_t state;

	if (!bc_tag_humidity_get_state(&tag_humidity, &state))
	{
		return;
	}

	switch (state)
	{
		case BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ:
		{

			if (!bc_tag_humidity_read_calibration(&tag_humidity))
			{
				return;
			}

			break;
		}
		case BC_TAG_HUMIDITY_STATE_POWER_DOWN:
		{
			if (!bc_tag_humidity_power_up(&tag_humidity))
			{
				return;
			}

			break;
		}
		case BC_TAG_HUMIDITY_STATE_POWER_UP:
		{
			if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
			{
		
				return;
			}

			break;
		}
		case BC_TAG_HUMIDITY_STATE_CONVERSION:
		{
			break;
		}
		case BC_TAG_HUMIDITY_STATE_RESULT_READY:
		{
            float value;

			if (!bc_tag_humidity_read_result(&tag_humidity))
			{
				return;
			}

			if (!bc_tag_humidity_get_result(&tag_humidity, &value))
			{
				return;
			}

			if (!bc_tag_humidity_one_shot_conversion(&tag_humidity))
			{
				return;
			}

			printf("humidity %f \n", value );

			break;
		}
		default:
		{

			return;
		}
	}
}


static void demo_temperature_init(void)
{
	bc_tag_temperature_init(&tag_temperature, bc_i2c_sys_get_tag_interface(), BC_TAG_TEMPERATURE_ADDRESS_DEFAULT);

	bc_tag_temperature_single_shot_conversion(&tag_temperature);
}

static void demo_temperature_task(void)
{
	bc_tag_temperature_state_t state;

	if (!bc_tag_temperature_get_state(&tag_temperature, &state))
	{
		return;
	}

	switch (state)
	{
		case BC_TAG_TEMPERATURE_STATE_POWER_DOWN:
		{
            float value;

			if (!bc_tag_temperature_read_temperature(&tag_temperature))
			{
				return;
			}

			if (!bc_tag_temperature_get_temperature_celsius(&tag_temperature, &value))
			{
				return;
			}

			if (!bc_tag_temperature_single_shot_conversion(&tag_temperature))
			{
				return;
			}

			printf("temperature %f \n", value );

			break;
		}
		case BC_TAG_TEMPERATURE_STATE_CONVERSION:
		{
			bc_tag_temperature_power_down(&tag_temperature);

			break;
		}
		default:
		{
			bc_tag_temperature_power_down(&tag_temperature);

			break;
		}
	}
}

int main (int argc, char *argv[])
{
    bc_tick_init();

    fprintf(stderr,"Bridge version %d.%d\n", Bridge_VERSION_MAJOR,Bridge_VERSION_MINOR );
    fprintf(stderr,"Bridge build %s \n", VERSION );

    if (!ft260_open_device()) {
        perror("Can not find the device");
        return EXIT_FAILURE;
    }

    printf("\tvendor: %d \n", ft260_check_chip_version() );

    
   //blink(5);

    //ft260_i2c_scan();

    //switch i2c 
    ft260_i2c_set_bus(SYS);

    // //vycteni z HTS221 WHO AM I 
    // unsigned char data[1];
    // data[0] = 0x0F;
    // ft260_i2c_write(0x5F, data, sizeof(data));
    // int res = ft260_i2c_read(0x5F, data, 1); //0x5F
    // printf("res %d \n", data[0] );

    


    demo_humidity_init();
    demo_temperature_init();
    while(1){
        demo_humidity_task();
        demo_temperature_task();
        sleep(1);
    }

    ft260_close_dev();
    
    return EXIT_SUCCESS;
}