#include "bc_i2c_tca9534a.h"
#include "bc_log.h"

static bool _bc_i2c_tca9534a_write_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t value);
static bool _bc_i2c_tca9534a_read_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t *value);

bool bc_i2c_tca9534a_init(bc_i2c_tca9534a_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

	self->_interface = interface;
	self->_device_address = device_address;
	self->_communication_fault = true;

	return true;
}

bool bc_i2c_tca9534a_read_port(bc_i2c_tca9534a_t *self, uint8_t *value)
{
    if (!_bc_i2c_tca9534a_read_register(self, 0x00, value))
    {
        bc_log_error("bc_i2c_tca9534a_read_port: call failed: _bc_ic2_tca9534a_read_register");

        return false;
    }

    return true;
}

bool bc_i2c_tca9534a_write_port(bc_i2c_tca9534a_t *self, uint8_t value)
{
    if (!_bc_i2c_tca9534a_write_register(self, 0x01, value))
    {
        bc_log_error("bc_i2c_tca9534a_write_port: call failed: _bc_ic2_tca9534a_write_register");

        return false;
    }

    return true;
}

bool bc_i2c_tca9534a_read_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t *value)
{
    uint8_t port;

    if (!bc_i2c_tca9534a_read_port(self, &port))
    {
        bc_log_error("bc_i2c_tca9534a_read_pin: call failed: bc_i2c_tca9534a_read_port");

        return false;
    }

    if (((port >> (uint8_t) pin) & 1) == 0)
    {
        *value = BC_I2C_TCA9534A_VALUE_LOW;
    }
    else
    {
        *value = BC_I2C_TCA9534A_VALUE_HIGH;
    }

    return true;
}

bool bc_i2c_tca9534a_write_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t value)
{
    uint8_t port;

    if (!bc_i2c_tca9534a_read_port(self, &port))
    {
        bc_log_error("bc_i2c_tca9534a_write_pin: call failed: bc_i2c_tca9534a_read_port");

        return false;
    }

    port &= ~(1 << (uint8_t) pin);

    if (value != BC_I2C_TCA9534A_VALUE_LOW)
    {
        port |= 1 << (uint8_t) pin;
    }

    if (!bc_i2c_tca9534a_write_port(self, port))
    {
        bc_log_error("bc_i2c_tca9534a_write_pin: call failed: bc_i2c_tca9534a_write_port");

        return false;
    }

    return true;
}

bool bc_i2c_tca9534a_get_port_direction(bc_i2c_tca9534a_t *self, uint8_t *direction)
{
    if (!_bc_i2c_tca9534a_read_register(self, 0x03, direction))
    {
        bc_log_error("bc_i2c_tca9534a_get_port_direction: call failed: _bc_i2c_tca9534a_read_register");

        return false;
    }

    return true;
}

bool bc_i2c_tca9534a_set_port_direction(bc_i2c_tca9534a_t *self, uint8_t direction)
{
    if (!_bc_i2c_tca9534a_write_register(self, 0x03, direction))
    {
        bc_log_error("bc_i2c_tca9534a_set_port_direction: call failed: _bc_i2c_tca9534a_write_register");

        return false;
    }

    return true;
}

bool bc_i2c_tca9534a_get_pin_direction(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin,
                                       bc_i2c_tca9534a_direction_t *direction)
{
    uint8_t port_direction;

    if (!bc_i2c_tca9534a_get_port_direction(self, &port_direction))
    {
        bc_log_error("bc_i2c_tca9534a_get_pin_direction: call failed: bc_i2c_tca9534a_get_port_direction");

        return false;
    }

    if (((port_direction >> (uint8_t) pin) & 1) == 0)
    {
        *direction = BC_I2C_TCA9534A_DIRECTION_OUTPUT;
    }
    else
    {
        *direction = BC_I2C_TCA9534A_DIRECTION_INPUT;
    }

    return true;
}

bool bc_i2c_tca9534a_set_pin_direction(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin,
                                       bc_i2c_tca9534a_direction_t direction)
{
    uint8_t port_direction;

    if (!bc_i2c_tca9534a_get_port_direction(self, &port_direction))
    {
        bc_log_error("bc_i2c_tca9534a_set_pin_direction: call failed: bc_i2c_tca9534a_get_port_direction");

        return false;
    }

    port_direction &= ~(1 << (uint8_t) pin);

    if (direction == BC_I2C_TCA9534A_DIRECTION_INPUT)
    {
        port_direction |= 1 << (uint8_t) pin;
    }

    if (!bc_i2c_tca9534a_set_port_direction(self, port_direction))
    {
        bc_log_error("bc_i2c_tca9534a_set_pin_direction: call failed: bc_i2c_tca9534a_set_port_direction");

        return false;
    }

    return true;
}

static bool _bc_i2c_tca9534a_write_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t value)
{
	bc_i2c_transfer_t transfer;

	uint8_t buffer[1];

    bc_i2c_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

	buffer[0] = value;

#ifdef BRIDGE

    self->_communication_fault = true;

    transfer.channel = self->_interface->channel;

    if (!bc_bridge_i2c_write(self->_interface->bridge, &transfer))
    {
        bc_log_error("_bc_i2c_tca9534a_write_register: call failed: bc_bridge_i2c_write");

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

static bool _bc_i2c_tca9534a_read_register(bc_i2c_tca9534a_t *self, uint8_t address, uint8_t *value)
{
	bc_i2c_transfer_t transfer;

	uint8_t buffer[1];

    bc_i2c_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

#ifdef BRIDGE

    self->_communication_fault = true;

    transfer.channel = self->_interface->channel;

    if (!bc_bridge_i2c_read(self->_interface->bridge, &transfer))
    {
        bc_log_error("_bc_i2c_tca9534a_read_register: call failed: bc_bridge_i2c_read");

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
