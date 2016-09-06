#include "bc_i2c_pca9535.h"

static bool _bc_ic2_pca9535_write_register(bc_i2c_pca9535_t *self, uint8_t address, uint8_t value);
static bool _bc_ic2_pca9535_read_register(bc_i2c_pca9535_t *self, uint8_t address, uint8_t *value);

bool bc_ic2_pca9535_init(bc_i2c_pca9535_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

	self->_interface = interface;
	self->_device_address = device_address;
	self->_communication_fault = true;
	return true;
}

bool bc_ic2_pca9535_read_pins(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t *pins)
{
    if (!_bc_ic2_pca9535_read_register(self, 0x00+port, pins)){
        return false;
    }
    return true;
}

bool bc_ic2_pca9535_write_pins(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t pins)
{
    return _bc_ic2_pca9535_write_register(self, 0x02+port, pins);
}

//bool bc_ic2_pca9535_read_pin(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_value_t *value)
//{
//    uint8_t pins;
//    if (!bc_ic2_pca9535_read_pins(self, &pins)){
//        return false;
//    }
//    *value = (pins >> pin) & 0x01;
//    return true;
//}
//
//bool bc_ic2_pca9535_write_pin(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_value_t value)
//{
//    uint8_t pins;
//    if (!bc_ic2_pca9535_read_pins(self, &pins)){
//        return false;
//    }
//
//    if (value==BC_I2C_pca9535_LOW){
//        pins &= (1 << pin) ^ 0xff;
//    }else{
//        pins |= (1 << pin);
//    }
//
//    return bc_ic2_pca9535_write_pins(self, pins);
//}

bool bc_ic2_pca9535_get_modes(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t *modes)
{
    if (!_bc_ic2_pca9535_read_register(self, 0x06+port, modes)){
        return false;
    }
    return true;
}

bool bc_ic2_pca9535_set_modes(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t modes)
{
    return _bc_ic2_pca9535_write_register(self, 0x06+port, modes);
}

//bool bc_ic2_pca9535_get_mode(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_mode_t *mode)
//{
//    uint8_t modes;
//    if (!bc_ic2_pca9535_get_modes(self, &modes)){
//        return false;
//    }
//    *mode = (modes >> pin) & 0x01;
//    return true;
//}
//
//bool bc_ic2_pca9535_set_mode(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_mode_t mode)
//{
//    uint8_t modes;
//    if (!bc_ic2_pca9535_get_modes(self, &modes)){
//        return false;
//    }
//    if(mode == BC_I2C_pca9535_OUTPUT)
//    {
//        modes &= (1 << pin) ^ 0xff;
//    }else{
//        modes |= (1 << pin);
//    }
//    return  bc_ic2_pca9535_set_modes(self, modes);
//}

static bool _bc_ic2_pca9535_write_register(bc_i2c_pca9535_t *self, uint8_t address, uint8_t value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[1];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

	buffer[0] = (uint8_t) value;

#ifdef BRIDGE
    self->_communication_fault = true;
    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_write(self->_interface->bridge, &transfer))
    {
        return false;
    }
    self->_communication_fault = false;

#else

	if (!self->_interface->write(&transfer, &self->_communication_fault))
	{
		return false;
	}
#endif

	return true;
}

static bool _bc_ic2_pca9535_read_register(bc_i2c_pca9535_t *self, uint8_t address, uint8_t *value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[1];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

#ifdef BRIDGE
    self->_communication_fault = true;
    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_read(self->_interface->bridge, &transfer))
    {
        return false;
    }
    self->_communication_fault = false;
#else
	if (!self->_interface->read(&transfer, &self->_communication_fault))
	{
		return false;
	}
#endif

	*value = buffer[0];

	return true;
}
