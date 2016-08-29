#include <bc/module/relay.h>
#include <bc/i2c/sys.h>

/* 
       N0_L  NC_H  NC_L  N0_H
 klid   0      1     0     1     0x50
 NC     0      0     1     1     0x30
 N0     1      1     0     0     0xc0

*/

static bool _bc_module_relay_set_mode(bc_module_relay_t *self, uint8_t pins);

void bc_module_relay_init(bc_module_relay_t *self, bc_tag_interface_t *interface)
{
	memset(self, 0, sizeof(*self));
	br_ic2_tca9534a_init(&self->_tca9534a, interface, 0x3B );
	_bc_module_relay_set_mode(self, 0x50);
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
    
    bc_os_sleep(10);
   
    if (!_bc_module_relay_set_mode(self, 0x50))
    {
        return false;
    }

    return true;
}

static bool _bc_module_relay_set_mode(bc_module_relay_t *self, uint8_t pins)
{
    if (!bc_ic2_tca9534a_set_modes(&self->_tca9534a, BC_I2C_TCA9534A_ALL_INPUT))
    {
        return false;
    }

    if (!bc_ic2_tca9534a_write_pins(&self->_tca9534a, pins )){
        return false;
    }

    if (!bc_ic2_tca9534a_set_modes(&self->_tca9534a, BC_I2C_TCA9534A_ALL_OUTPUT))
    {
        return false;
    }

    return true;
}