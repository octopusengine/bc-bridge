#include "bc_tag_humidity.h"
#include "chip_hts221.h"

// TODO Define bit mask for individual registers
// TODO Implement continuous conversion
// TODO Review and optimize calculation (linear interpolation)

static bool _bc_tag_humidity_write_register(bc_tag_humidity_t *self, uint8_t address, uint8_t value);
static bool _bc_tag_humidity_read_register(bc_tag_humidity_t *self, uint8_t address, uint8_t *value);

bool bc_tag_humidity_init(bc_tag_humidity_t *self, bc_tag_interface_t *interface)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_communication_fault = true;
    self->_calibration_not_read = true;

    if (!bc_tag_humidity_power_down(self))
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_is_communication_fault(bc_tag_humidity_t *self)
{
    return self->_communication_fault;
}

bool bc_tag_humidity_get_state(bc_tag_humidity_t *self, bc_tag_humidity_state_t *state)
{
    uint8_t ctrl_reg1;
    uint8_t ctrl_reg2;
    uint8_t status_reg;

    if (self->_calibration_not_read)
    {
        *state = BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ;

        return true;
    }

    if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG1, &ctrl_reg1))
    {
        return false;
    }

    // TODO Power down bit?
    if ((ctrl_reg1 & 0x80) == 0)
    {
        *state = BC_TAG_HUMIDITY_STATE_POWER_DOWN;

        return true;
    }

    *state = BC_TAG_HUMIDITY_STATE_CONVERSION;

    if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG2, &ctrl_reg2))
    {
        return false;
    }

    // TODO One-shot bit?
    if ((ctrl_reg2 & 0x01) != 0)
    {
        return true;
    }

    if (!_bc_tag_humidity_read_register(self, HTS221_STATUS_REG, &status_reg))
    {
        return false;
    }

    // TODO Humidity ready bit?
    if ((status_reg & 0x02) != 0)
    {
        *state = BC_TAG_HUMIDITY_STATE_RESULT_READY;

        return true;
    }

    // TODO One-shot mode is not enabled?
    if ((ctrl_reg1 & 0x03) != 0)
    {
        return true;
    }

    *state = BC_TAG_HUMIDITY_STATE_POWER_UP;

    return true;
}

bool bc_tag_humidity_read_calibration(bc_tag_humidity_t *self)
{
    uint8_t i;

    for (i = 0; i < 16; i++)
    {
        if (!_bc_tag_humidity_read_register(self, HTS221_CALIB_OFFSET + i, &self->_calibration[i]))
        {
            return false;
        }
    }

    self->_calibration_not_read = false;

    return true;
}

bool bc_tag_humidity_power_up(bc_tag_humidity_t *self)
{
    if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, 0x80))
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_power_down(bc_tag_humidity_t *self)
{
    if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, 0x00))
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_one_shot_conversion(bc_tag_humidity_t *self)
{
    if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, 0x84))
    {
        return false;
    }

    if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG2, 0x01))
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_read_result(bc_tag_humidity_t *self)
{
    if (!_bc_tag_humidity_read_register(self, HTS221_HUMIDITY_OUT_L, &self->_humidity_out_lsb))
    {
        return false;
    }

    if (!_bc_tag_humidity_read_register(self, HTS221_HUMIDITY_OUT_H, &self->_humidity_out_msb))
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_get_result(bc_tag_humidity_t *self, float *humidity)
{
    int16_t h0_rh;
    int16_t h1_rh;
    int16_t h0_t0_out;
    int16_t h1_t0_out;
    int16_t h_t_out;

    h0_rh = (int16_t) self->_calibration[0];
    h0_rh >>= 1;

    h1_rh = (int16_t) self->_calibration[1];
    h1_rh >>= 1;

    h0_t0_out = (int16_t) self->_calibration[6];
    h0_t0_out |= ((int16_t) self->_calibration[7]) << 8;

    h1_t0_out = (int16_t) self->_calibration[10];
    h1_t0_out |= ((int16_t) self->_calibration[11]) << 8;

    if ((h1_t0_out - h0_t0_out) == 0)
    {
        return false;
    }

    h_t_out = (int16_t) self->_humidity_out_lsb;
    h_t_out |= ((int16_t) self->_humidity_out_msb) << 8;

    *humidity = (float) (((((((int32_t) h1_rh) - ((int32_t) h0_rh)) * (((int32_t) h_t_out) - ((int32_t) h0_t0_out))) * 10) / (((int32_t) h1_t0_out) - ((int32_t) h0_t0_out))) + (10 * (int32_t) h0_rh));
    *humidity /= 10.f;

    if (*humidity >= 100.f)
    {
        *humidity = 100.f;
    }

    return true;
}

static bool _bc_tag_humidity_write_register(bc_tag_humidity_t *self, uint8_t address, uint8_t value)
{
    bc_tag_transfer_t transfer;

    uint8_t buffer[1];
    buffer[0] = value;

    bc_tag_transfer_init(&transfer);

    transfer.device_address = BC_TAG_HUMIDITY_DEVICE_ADDRESS;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

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

static bool _bc_tag_humidity_read_register(bc_tag_humidity_t *self, uint8_t address, uint8_t *value)
{
    bc_tag_transfer_t transfer;

    uint8_t buffer[1];

    bc_tag_transfer_init(&transfer);

    transfer.device_address = BC_TAG_HUMIDITY_DEVICE_ADDRESS;
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
