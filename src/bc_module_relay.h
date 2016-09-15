#ifndef _BC_MODULE_RELAY_H
#define _BC_MODULE_RELAY_H

#include "bc_common.h"
#include "bc_i2c.h"
#include "bc_i2c_tca9534a.h"

#define BC_MODULE_RELAY_ADDRESS_DEFAULT 0x3B
#define BC_MODULE_RELAY_ADDRESS_ALTERNATE 0x3B

typedef enum
{
     BC_MODULE_RELAY_STATE_T = 0,
     BC_MODULE_RELAY_STATE_F = 1

} bc_module_relay_state_t;

typedef struct
{
    bc_i2c_tca9534a_t _tca9534a;

} bc_module_relay_t;

bool bc_module_relay_init(bc_module_relay_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_module_relay_set_state(bc_module_relay_t *self, bc_module_relay_state_t state);

#endif /* _BC_MODULE_RELAY_H */
