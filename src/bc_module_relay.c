#include "bc_module_relay.h"

/*
       N0_L  NC_H  NC_L  N0_H
 klid   0      1     0     1     0x50
 NC     0      0     1     1     0x30
 N0     1      1     0     0     0xc0

*/

static bool _bc_module_relay_set_mode(bc_module_relay_t *self, uint8_t pins);

bool bc_module_relay_init(bc_module_relay_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

    if (!bc_i2c_tca9534a_init(&self->_tca9534a, interface, device_address)){
        return false;
    }

    return _bc_module_relay_set_mode(self, 0x50);
}

bool bc_module_relay_set_mode(bc_module_relay_t *self, bc_module_relay_mode_t relay_mode)
{
    uint8_t pins = 0x30; //BC_MODULE_RELAY_MODE_NC

    if (relay_mode == BC_MODULE_RELAY_MODE_NO)
    {
        pins = 0xc0;
    }

    if (!_bc_module_relay_set_mode(self, pins))
    {
        return false;
    }

    bc_os_task_sleep(10);

    if (!_bc_module_relay_set_mode(self, 0x50))
    {
        return false;
    }

    return true;
}

static bool _bc_module_relay_set_mode(bc_module_relay_t *self, uint8_t pins)
{
    if (!bc_i2c_tca9534a_set_port_direction(&self->_tca9534a, BC_I2C_TCA9534A_DIRECTION_ALL_INPUT))
    {
        return false;
    }

    if (!bc_i2c_tca9534a_write_port(&self->_tca9534a, pins)){
        return false;
    }

    if (!bc_i2c_tca9534a_set_port_direction(&self->_tca9534a, BC_I2C_TCA9534A_DIRECTION_ALL_OUTPUT))
    {
        return false;
    }

    return true;
}
