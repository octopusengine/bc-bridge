#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bridge.h"
#include "jsmn.h"
#include "ft260.h"


int main (int argc, char *argv[])
{
    fprintf(stderr,"Bridge version %d.%d\n", Bridge_VERSION_MAJOR,Bridge_VERSION_MINOR );
    fprintf(stderr,"Bridge build %s \n", VERSION );

    char *device_path = NULL;
    device_path = get_hid_path(0x0403, 0x6030, 0);

    if (!device_path) {
        perror("Can not find the device path");
        return EXIT_FAILURE;
    }

    
    return EXIT_SUCCESS;
}