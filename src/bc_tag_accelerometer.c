#include "bc_tag_accelerometer.h"

static bool _bc_tag_accelerometer_write_register(bc_tag_accelerometer_t *self, uint8_t address, uint8_t value);
static bool _bc_tag_accelerometer_read_register(bc_tag_accelerometer_t *self, uint8_t address, uint8_t *value);

bool bc_tag_accelerometer_init(bc_tag_accelerometer_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

	self->_interface = interface;
	self->_device_address = device_address;
	self->_communication_fault = true;

	if (!bc_tag_accelerometer_power_down(self))
	{
		return false;
	}

	return true;
}

bool bc_tag_accelerometer_is_communication_fault(bc_tag_accelerometer_t *self)
{
	return self->_communication_fault;
}

bool bc_tag_accelerometer_get_state(bc_tag_accelerometer_t *self, bc_tag_accelerometer_state_t *state)
{
	uint8_t value;

	if (!_bc_tag_accelerometer_read_register(self, 0x20, &value))
	{
		return false;
	}

	if ((value & 0xF0) == 0)
	{
		*state = BC_TAG_ACCELEROMETER_STATE_POWER_DOWN;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x27, &value))
	{
		return false;
	}

	if ((value & 0x08) != 0)
	{
		*state = BC_TAG_ACCELEROMETER_STATE_RESULT_READY;
	}
	else
	{
		*state = BC_TAG_ACCELEROMETER_STATE_CONVERSION;
	}

	return true;
}

bool bc_tag_accelerometer_power_down(bc_tag_accelerometer_t *self)
{
	if (!_bc_tag_accelerometer_write_register(self, 0x20, 0x07))
	{
		return false;
	}

	return true;
}

bool bc_tag_accelerometer_continuous_conversion(bc_tag_accelerometer_t *self)
{
	if (!_bc_tag_accelerometer_write_register(self, 0x23, 0x98))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_write_register(self, 0x20, 0x27))
	{
		return false;
	}

	return true;
}

bool bc_tag_accelerometer_read_result(bc_tag_accelerometer_t *self)
{
	if (!_bc_tag_accelerometer_read_register(self, 0x28, &self->_out_x_l))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x29, &self->_out_x_h))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x2A, &self->_out_y_l))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x2B, &self->_out_y_h))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x2C, &self->_out_z_l))
	{
		return false;
	}

	if (!_bc_tag_accelerometer_read_register(self, 0x2D, &self->_out_z_h))
	{
		return false;
	}

	return true;
}

bool bc_tag_accelerometer_get_result_raw(bc_tag_accelerometer_t *self, bc_tag_accelerometer_result_raw_t *result_raw)
{
	result_raw->x_axis = (int16_t) self->_out_x_h;
	result_raw->x_axis <<= 8;
	result_raw->x_axis >>= 4;
	result_raw->x_axis |= ((int16_t) self->_out_x_l) >> 4;

	result_raw->y_axis = (int16_t) self->_out_y_h;
	result_raw->y_axis <<= 8;
	result_raw->y_axis >>= 4;
	result_raw->y_axis |= ((int16_t) self->_out_y_l) >> 4;

	result_raw->z_axis = (int16_t) self->_out_z_h;
	result_raw->z_axis <<= 8;
	result_raw->z_axis >>= 4;
	result_raw->z_axis |= ((int16_t) self->_out_z_l) >> 4;

	return true;
}

bool bc_tag_accelerometer_get_result_g(bc_tag_accelerometer_t *self, bc_tag_accelerometer_result_g_t *result_g)
{
	bc_tag_accelerometer_result_raw_t result_raw;

	if (!bc_tag_accelerometer_get_result_raw(self, &result_raw))
	{
		return false;
	}

	result_g->x_axis = ((float) result_raw.x_axis) / 512.f;
	result_g->y_axis = ((float) result_raw.y_axis) / 512.f;
	result_g->z_axis = ((float) result_raw.z_axis) / 512.f;

	return true;
}

static bool _bc_tag_accelerometer_write_register(bc_tag_accelerometer_t *self, uint8_t address, uint8_t value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[1];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 1;

	buffer[0] = value;

	if (!self->_interface->write(&transfer, &self->_communication_fault))
	{
		return false;
	}

	return true;
}

static bool _bc_tag_accelerometer_read_register(bc_tag_accelerometer_t *self, uint8_t address, uint8_t *value)
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
