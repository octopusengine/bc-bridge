#include <bc/i2c/tca9534a.h>

static bool _bc_ic2_tca9534a_write_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t value);
static bool _bc_ic2_tca9534a_read_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t *value);

bool br_ic2_tca9534a_init(bc_i2c_tca9534a_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

	self->_interface = interface;
	self->_device_address = device_address;
	self->_communication_fault = true;
	return true;
}

bool bc_ic2_tca9534a_read_pins(bc_i2c_tca9534a_t *self, uint8_t *pins)
{
    if(!_bc_ic2_tca9534a_read_register(self, 0x00, pins)){
        return false;
    }
    return true;
}

bool bc_ic2_tca9534a_read_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t *value)
{
    uint8_t pins;
    if(!bc_ic2_tca9534a_read_pins(self, &pins)){
        return false;
    }
    *value = pins & pin;
    return true;
}


bool bc_ic2_tca9534a_write_pins(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t value)
{

}

bool bc_ic2_tca9534a_write_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t value)
{

}

static bool _bc_ic2_tca9534a_write_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[1];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

	buffer[0] = (uint8_t) value;

	if (!self->_interface->write(&transfer, &self->_communication_fault))
	{
		return false;
	}

	return true;
}

static bool _bc_ic2_tca9534a_read_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t *value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[1];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

	if (!self->_interface->read(&transfer, &self->_communication_fault))
	{
		return false;
	}

	*value = buffer[0];

	return true;
}
