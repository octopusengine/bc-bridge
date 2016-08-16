#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bridge.h"
#include "jsmn.h"
#include "ft260.h"

void blink(int cnt){
    for(int i=0;i<cnt;i++){
        ft260_led(i%2==0);
        sleep(1);
    }
    ft260_led(0);
}

int main (int argc, char *argv[])
{
    fprintf(stderr,"Bridge version %d.%d\n", Bridge_VERSION_MAJOR,Bridge_VERSION_MINOR );
    fprintf(stderr,"Bridge build %s \n", VERSION );

    if (!ft260_open_device()) {
        perror("Can not find the device");
        return EXIT_FAILURE;
    }

    printf("\tvendor: %d \n", ft260_check_chip_version() );

    // blink(5);

    ft260_i2c_scan();


    //vycteni z HTS221 WHO AM I 
    unsigned char data[1];

    //switch i2c 
    data[0] = 0x01;
    ft260_i2c_write(112, data, sizeof(data));

    data[0] = 0x0F;
    ft260_i2c_write(0x5F, data, sizeof(data));
    int res = ft260_i2c_read(0x5F, data, 1); //0x5F
    printf("res %d \n", data[0] );

    ft260_close_dev();
    
    return EXIT_SUCCESS;
}