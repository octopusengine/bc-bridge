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
#include <bc/i2c/tca9534a.h>
#include <bc/tag/humidity.h>
#include <bc/tag/temperature.h>
#include <bc/module/co2.h>
#include <bc/module/relay.h>

void blink(int cnt){
    for(int i=0;i<cnt;i++){
        ft260_led(i%2==0);
        sleep(1);
    }
    ft260_led(0);
}

bc_tag_humidity_t tag_humidity;
bc_tag_temperature_t tag_temperature;
bc_module_co2_t module_co2;
bc_module_relay_t module_relay;

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

			printf("[\"humidity-sensor\", {\"0/relative-humidity\":[%f, \"%%\"]}]\n", value );

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

			printf("[\"thermometer\", {\"0/temperature\": [%f, \"\\u2103\"]\n", value );

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

static void demo_co2_init(void)
{
	bc_module_co2_init(&module_co2);
}

static void demo_co2_task(void)
{
	int16_t co2_concentration;

	bc_module_co2_task(&module_co2);

	if (bc_module_co2_get_concentration(&module_co2, &co2_concentration))
	{
		printf("[\"co2-sensor\",{\"concentration\":[%d,\"ppm\"]}]\n", co2_concentration );
	}
}

int main (int argc, char *argv[])
{

	uint8_t bus_status;

    fprintf(stderr,"Bridge version %d.%d\n", Bridge_VERSION_MAJOR,Bridge_VERSION_MINOR );
    fprintf(stderr,"Bridge build %s \n", VERSION );

    if (!ft260_open()) {
        perror("Can not find the device\n");
        return EXIT_FAILURE;
    }

    printf("chip_version: %d \n", ft260_check_chip_version() );
	
	ft260_uart_set_default_configuration();
	//ft260_uart_print_configuration();

	ft260_i2c_set_clock_speed(200);

	//Pro jistotu reset i2c a pak kontrola na i2c switch
   	if (!ft260_i2c_reset() || !ft260_i2c_is_device_exists(0x70) ){
		
		ft260_i2c_get_bus_status(&bus_status);

		perror("Error on i2c bus : it is necessary to disconnect and reconnect the device\n");
		fprintf(stderr, "bus_status value dec %d \n", bus_status);
		return EXIT_FAILURE;
	}

	ft260_i2c_set_bus(FT260_I2C_BUS_0);


	// bc_i2c_tca9534a_t tca9534a;
	// br_ic2_tca9534a_init(&tca9534a, bc_i2c_sys_get_tag_interface(), 0x38);

	// bc_ic2_tca9534a_set_mode(&tca9534a, BC_I2C_TCA9534A_PIN0, BC_I2C_TCA9534A_OUTPUT);

	// bc_i2c_tca9534a_mode_t mode;
	// bc_ic2_tca9534a_get_mode(&tca9534a, BC_I2C_TCA9534A_PIN0, &mode);

	// printf("mode: %d \n", mode );

	// bc_ic2_tca9534a_write_pin(&tca9534a, BC_I2C_TCA9534A_PIN0, 1);

	// bc_i2c_tca9534a_value_t value;
	// bc_ic2_tca9534a_read_pin(&tca9534a, BC_I2C_TCA9534A_PIN1, &value);
    // printf("value: %d \n", value );



	
	
	//printf("ft260_i2c_get_clock_speed: %d \n", ft260_i2c_get_clock_speed() );
	// ft260_i2c_set_clock_speed(100);
	// uint32_t speed;
	// ft260_i2c_get_clock_speed(&speed);
	// printf("ft260_i2c_get_clock_speed: %d \n", speed );

	//switch i2c 
    // ft260_i2c_set_bus(FT260_I2C_BUS_0);
	// uint8_t bus_status;
	// uint8_t buffer[4];

	// ft260_i2c_write(0x38, buffer, 1 );
	// ft260_i2c_get_bus_status(&bus_status);
	// printf("ft260_i2c_get_bus_status: %d \n", bus_status & 0x04 );

    ft260_i2c_scan();

	// printf("humidity tag %d \n", ft260_i2c_is_device_exists( 0x5F ) );
	
	
	// for (int i=0; i<10; i++)
	// {
	// 	ft260_i2c_get_bus_status(&bus_status);
	// 	fprintf(stderr, "bus_status value dec %d \n", bus_status);
	// 	usleep(100*1000);
	// }


	//ft260_uart_reset();

	// uint8_t buffer_out[64];
	// buffer_out[0] = 30;
	// buffer_out[1] = 32;
	// buffer_out[2] = 34;
	// buffer_out[3] = 36;
	
	// ft260_uart_write(buffer_out,4);
	
	// uint8_t buffer_in[64];
	// memset(buffer_in, 0, 64);

	// ft260_uart_read(buffer_in,4, 100);
	// print_buffer(buffer_in, 4);
	
	// return EXIT_SUCCESS;

    
    // //vycteni z HTS221 WHO AM I 
    // unsigned char data[1];
    // data[0] = 0x0F;
    // ft260_i2c_write(0x5F, data, sizeof(data));
    // int res = ft260_i2c_read(0x5F, data, 1); //0x5F
    // printf("res %d \n", data[0] );
	
	bc_module_relay_init(&module_relay);

	//demo_co2_init();

    // demo_humidity_init();
    //demo_temperature_init();
	
	int diode = 0;
	while(1){
		//demo_co2_task();
    
		diode = !diode ? 1 : 0;
        // demo_humidity_task();
		ft260_led(diode);
		bc_module_relay_set_mode(&module_relay, diode ? BC_MODULE_RELAY_MODE_NO : BC_MODULE_RELAY_MODE_NC);
		// demo_temperature_task();
		
	    sleep(1);
		
	//printf("------------------------------------\n");
    }

    ft260_close();
    
    return EXIT_SUCCESS;
}
