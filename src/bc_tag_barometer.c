#include "bc_tag_barometer.h"
#include "chip_mpl3115a2.h"
#include "bc_bridge.h"

static bool _bc_tag_barometer_read_result(bc_tag_barometer_t *self);
static bool _bc_tag_barometer_write_register(bc_tag_barometer_t *self, uint8_t address, uint8_t value);
static bool _bc_tag_barometer_read_register(bc_tag_barometer_t *self, uint8_t address, uint8_t *value);
static bc_tick_t _minimal_measurement_interval[] = { 6, 10, 18, 34, 66, 130, 258, 512 };

bool bc_tag_barometer_init(bc_tag_barometer_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
    uint8_t who_am_i;
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;

    self->disable_log = true;

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_WHO_AM_I, &who_am_i))
    {
        return false;
    }

    if (who_am_i != MPL3115A2_WHO_AM_I_RESULT)
    {
        return false;
    }

    self->disable_log = false;

    if (!bc_tag_barometer_power_down(self))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_get_minimal_measurement_interval(bc_tag_barometer_t *self, bc_tick_t *interval)
{
    uint8_t ctrl_reg1;
    if (!_bc_tag_barometer_read_register(self, MPL3115A2_CTRL_REG1, &ctrl_reg1))
    {
        return false;
    }

    *interval = _minimal_measurement_interval[(ctrl_reg1 & MPL3115A2_MASK_OS) >> 3];
    return true;
}

bool bc_tag_barometer_is_communication_fault(bc_tag_barometer_t *self)
{
    return self->_communication_fault;
}

bool bc_tag_barometer_get_state(bc_tag_barometer_t *self, bc_tag_barometer_state_t *state)
{
    uint8_t status;
    uint8_t ctrl_reg1;

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_CTRL_REG1, &ctrl_reg1))
    {
        return false;
    }

    if ((ctrl_reg1 & MPL3115A2_BIT_SBYB) == 0)
    {

        *state = BC_TAG_BAROMETER_STATE_POWER_DOWN;

        return true;
    }

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_STATUS, &status))
    {
        return false;
    }

    if ((status & MPL3115A2_BIT_PTDR) != 0)
    {
        if ((ctrl_reg1 & MPL3115A2_BIT_ALT) != 0)
        {
            *state = BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE;
        }
        else
        {
            *state = BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE;
        }

    }
    else
    {
        *state = BC_TAG_BAROMETER_STATE_CONVERSION;
    }

    return true;
}

bool bc_tag_barometer_power_down(bc_tag_barometer_t *self)
{
    uint8_t ctrl_reg1;

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_CTRL_REG1, &ctrl_reg1))
    {
        return false;
    }

    ctrl_reg1 &= ~MPL3115A2_BIT_SBYB;

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, ctrl_reg1))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_reset_and_power_down(bc_tag_barometer_t *self)
{

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, MPL3115A2_BIT_RST))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_one_shot_conversion_altitude(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0xB8))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_PT_DATA_CFG,
                                          MPL3115A2_BIT_TDEFE | MPL3115A2_BIT_PDEFE | MPL3115A2_BIT_DREM))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0xBB))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_one_shot_conversion_pressure(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0x38))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_PT_DATA_CFG,
                                          MPL3115A2_BIT_TDEFE | MPL3115A2_BIT_PDEFE | MPL3115A2_BIT_DREM))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0x3B))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_continuous_conversion_altitude(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0xB8))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_PT_DATA_CFG,
                                          MPL3115A2_BIT_TDEFE | MPL3115A2_BIT_PDEFE | MPL3115A2_BIT_DREM))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0xB9))
    {
        return false;
    }

    return true;
}

bool bc_tag_barometer_continuous_conversion_pressure(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0x38))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_PT_DATA_CFG,
                                          MPL3115A2_BIT_TDEFE | MPL3115A2_BIT_PDEFE | MPL3115A2_BIT_DREM))
    {
        return false;
    }

    if (!_bc_tag_barometer_write_register(self, MPL3115A2_CTRL_REG1, 0x39))
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

    out_p = (uint32_t) self->_out_p_lsb;
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

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_OUT_T_MSB, &self->_out_t_msb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_OUT_T_LSB, &self->_out_t_lsb))
    {
        return false;
    }

    out_t = (uint16_t) (self->_out_t_msb << 8) | (uint16_t) (self->_out_t_lsb);

    *temperature = ((float) out_t) / 256.f;

    return true;
}

static bool _bc_tag_barometer_read_result(bc_tag_barometer_t *self)
{
    if (!_bc_tag_barometer_read_register(self, MPL3115A2_OUT_P_MSB, &self->_out_p_msb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_OUT_P_CSB, &self->_out_p_csb))
    {
        return false;
    }

    if (!_bc_tag_barometer_read_register(self, MPL3115A2_OUT_P_LSB, &self->_out_p_lsb))
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

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

    buffer[0] = value;

#ifdef BRIDGE
    transfer.disable_log = self->disable_log;

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

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

#ifdef BRIDGE
    transfer.disable_log = self->disable_log;

    transfer.disable_log = address == MPL3115A2_WHO_AM_I;

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
