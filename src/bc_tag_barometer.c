#include "bc_tag_barometer.h"

static bool _bc_tag_barometer_read_result(bc_tag_barometer_t *self);
static bool _bc_tag_barometer_write_register(bc_tag_barometer_t *self, uint8_t address, uint8_t value);
static bool _bc_tag_barometer_read_register(bc_tag_barometer_t *self, uint8_t address, uint8_t *value);
static bc_tick_t _minimal_measurement_interval[] = {6, 10, 18, 34, 66, 130, 258, 512};

bool bc_tag_barometer_init(bc_tag_barometer_t *self, bc_i2c_interface_t *interface)
{
    uint8_t who_am_i;
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_communication_fault = true;

    if (!_bc_tag_barometer_read_register(self, 0x0C, &who_am_i))
    {
        return false;
    }

    if (who_am_i != 0xC4)
    {
        return false;
    }

    if (!bc_tag_barometer_power_down(self))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_get_minimal_measurement_interval(bc_tag_barometer_t *self, bc_tick_t *interval)
{
    uint8_t ctrl_reg1;
    if (!_bc_tag_barometer_read_register(self, 0x26, &ctrl_reg1))
    {
        return false;
    }

    *interval = _minimal_measurement_interval[(ctrl_reg1 >> 3) & 0x07];
    return true;
}

bool bc_tag_barometer_is_communication_fault(bc_tag_barometer_t *self)
{
    return self->_communication_fault;
}

bool bc_tag_barometer_get_state(bc_tag_barometer_t *self, bc_tag_barometer_state_t *state)
{
    // TODO Review
    uint8_t status;
    uint8_t ctrl_reg1;

    if (!_bc_tag_barometer_read_register(self, 0x00, &status))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, 0x26, &ctrl_reg1))
    {
        return false;
    }

    if ((status & 0x08) != 0)
    {
        if ((ctrl_reg1 & 0x80) != 0)
        {
            *state = BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE;
        }
        else
        {
            *state = BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE;
        }

        return true;
    }

    if ((ctrl_reg1 & 0x01) != 0)
    {
        *state = BC_TAG_BAROMETER_STATE_CONVERSION;

        return true;
    }

    *state = BC_TAG_BAROMETER_STATE_POWER_DOWN;

    return true;
}

bool bc_tag_barometer_power_down(bc_tag_barometer_t *self)
{
    // TODO Implement
    return true;
}

bool bc_tag_barometer_one_shot_conversion_altitude(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, 0x26, 0xB8))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x13, 0x07))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x26, 0xBA))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_one_shot_conversion_pressure(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, 0x26, 0x38))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x13, 0x07))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x26, 0x3A))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_continuous_conversion_altitude(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, 0x26, 0xB8))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x13, 0x07))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x26, 0xB9))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_continuous_conversion_pressure(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, 0x26, 0x38))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x13, 0x07))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, 0x26, 0x39))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_get_altitude(bc_tag_barometer_t *self, float *altitude_meter)
{
    uint32_t out_p;

    if (!_bc_tag_barometer_read_result(self))
    {
        return false;
    }

    out_p = (uint32_t) self->_out_p_lsb ;
    out_p |= ((uint32_t) self->_out_p_csb) << 8;
    out_p |= ((uint32_t) self->_out_p_msb) << 16;

    *altitude_meter = ((float) out_p) / 256.f;

    return true;
}

bool bc_tag_barometer_get_pressure(bc_tag_barometer_t *self, float *pressure_pascal)
{
    uint32_t out_p;

    if (!_bc_tag_barometer_read_result(self))
    {
        return false;
    }

    out_p = (uint32_t) self->_out_p_lsb;
    out_p |= ((uint32_t) self->_out_p_csb) << 8;
    out_p |= ((uint32_t) self->_out_p_msb) << 16;

    *pressure_pascal = ((float) out_p) / 64.f;

    return true;
}

bool bc_tag_barometer_get_temperature(bc_tag_barometer_t *self, float *temperature)
{
    uint32_t out_t;

    if (!_bc_tag_barometer_read_register(self, 0x04, &self->_out_t_msb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, 0x05, &self->_out_t_lsb))
    {
        return false;
    }

    out_t = (uint16_t)(self->_out_t_msb << 8) | (uint16_t)( self->_out_t_lsb );

    *temperature = ((float) out_t) / 256.f;

    return true;
}

static bool _bc_tag_barometer_read_result(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_read_register(self, 0x01, &self->_out_p_msb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, 0x02, &self->_out_p_csb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, 0x03, &self->_out_p_lsb))
    {
        return false;
    }

    return true;
}

static bool _bc_tag_barometer_write_register(bc_tag_barometer_t *self, uint8_t address, uint8_t value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = 0x60;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

    buffer[0] = value;

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

static bool _bc_tag_barometer_read_register(bc_tag_barometer_t *self, uint8_t address, uint8_t *value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = 0x60;
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
