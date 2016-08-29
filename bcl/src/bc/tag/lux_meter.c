#include <bc/tag/lux_meter.h>
#include <bc/tag/chip/opt3001.h>

static bool _bc_tag_lux_meter_write_register(bc_tag_lux_meter_t *self, uint8_t address, uint16_t value);
static bool _bc_tag_lux_meter_read_register(bc_tag_lux_meter_t *self, uint8_t address, uint16_t *value);

bool bc_tag_lux_meter_init(bc_tag_lux_meter_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
	memset(self, 0, sizeof(*self));

	self->_interface = interface;
	self->_device_address = device_address;
	self->_communication_fault = true;
	self->_configuration = OPT3001_BIT_RN3 | OPT3001_BIT_RN2 | OPT3001_BIT_CT | OPT3001_BIT_L;

	if (!bc_tag_lux_meter_power_down(self))
	{
		return false;
	}

	return true;
}

bool bc_tag_lux_meter_is_communication_fault(bc_tag_lux_meter_t *self)
{
	return self->_communication_fault;
}

bool bc_tag_lux_meter_get_state(bc_tag_lux_meter_t *self, bc_tag_lux_meter_state_t *state)
{
	uint16_t configuration;

	if (!_bc_tag_lux_meter_read_register(self, OPT3001_REG_CONFIGURATION, &configuration))
	{
		return false;
	}

	if ((configuration & OPT3001_BIT_CRF) != 0)
	{
		*state = BC_TAG_LUX_METER_STATE_RESULT_READY;
	}
	else
	{
		if ((configuration & OPT3001_MASK_M) == 0)
		{
			*state = BC_TAG_LUX_METER_STATE_POWER_DOWN;
		}
		else
		{
			*state = BC_TAG_LUX_METER_STATE_CONVERSION;
		}
	}

	return true;
}

bool bc_tag_lux_meter_power_down(bc_tag_lux_meter_t *self)
{
	uint16_t configuration;

	configuration = self->_configuration;
	configuration &= ~OPT3001_MASK_M;

	if (!_bc_tag_lux_meter_write_register(self, OPT3001_REG_CONFIGURATION, configuration))
	{
		return false;
	}

	return true;
}

bool bc_tag_lux_meter_single_shot_conversion(bc_tag_lux_meter_t *self)
{
	uint16_t configuration;

	configuration = self->_configuration;
	configuration &= ~OPT3001_MASK_M;
	configuration |= OPT3001_BIT_M0;

	if (!_bc_tag_lux_meter_write_register(self, OPT3001_REG_CONFIGURATION, configuration))
	{
		return false;
	}

	return true;
}

bool bc_tag_lux_meter_continuous_conversion(bc_tag_lux_meter_t *self)
{
	uint16_t configuration;

	configuration = self->_configuration;
	configuration &= ~OPT3001_MASK_M;
	configuration |= OPT3001_BIT_M1 | OPT3001_BIT_M0;

	if (!_bc_tag_lux_meter_write_register(self, OPT3001_REG_CONFIGURATION, configuration))
	{
		return false;
	}

	return true;
}

bool bc_tag_lux_meter_read_result(bc_tag_lux_meter_t *self)
{
	uint16_t result;

	if (!_bc_tag_lux_meter_read_register(self, OPT3001_REG_RESULT, &result))
	{
		return false;
	}

	self->_result = result;

	return true;
}

bool bc_tag_lux_meter_get_result_lux(bc_tag_lux_meter_t *self, float *result_lux)
{
	uint8_t e;
	uint16_t r;

	e = (self->_result & OPT3001_MASK_E) >> 12;
	r = self->_result & OPT3001_MASK_R;

	*result_lux = (0.01f * (float) (((uint16_t) 1) << e)) * (float) r;

	return true;
}

static bool _bc_tag_lux_meter_write_register(bc_tag_lux_meter_t *self, uint8_t address, uint16_t value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[2];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 2;

	buffer[0] = (uint8_t) (value >> 8);
	buffer[1] = (uint8_t) value;
#ifdef BRIDGE
    self->_communication_fault = true;
    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_write_register( self->_interface->bridge, &transfer))
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

static bool _bc_tag_lux_meter_read_register(bc_tag_lux_meter_t *self, uint8_t address, uint16_t *value)
{
	bc_tag_transfer_t transfer;

	uint8_t buffer[2];

	bc_tag_transfer_init(&transfer);

	transfer.device_address = self->_device_address;
	transfer.buffer = buffer;
	transfer.address = address;
	transfer.length = 2;

#ifdef BRIDGE
    self->_communication_fault = true;
    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_read_register( self->_interface->bridge, &transfer))
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

	*value = (uint16_t) buffer[1];
	*value |= ((uint16_t) buffer[0]) << 8;

	return true;
}
