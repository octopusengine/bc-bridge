#ifndef _BC_MODULE_RELAY_H
#define _BC_MODULE_RELAY_H

#include "bc_common.h"
#include "bc_i2c.h"
#include "bc_i2c_tca9534a.h"

#define BC_MODULE_RELAY_ADDRESS_DEFAULT 0x3B
#define BC_MODULE_RELAY_ADDRESS_ALTERNATE 0x3B

typedef enum
{
     BC_MODULE_RELAY_MODE_NO = 0, //red led
     BC_MODULE_RELAY_MODE_NC = 1  //green led

} bc_module_relay_mode_t;

typedef struct
{
    bc_i2c_tca9534a_t _tca9534a;

} bc_module_relay_t;

void bc_module_relay_init(bc_module_relay_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_module_relay_set_mode(bc_module_relay_t *self, bc_module_relay_mode_t relay_mode);

#endif /* _BC_MODULE_RELAY_H */
