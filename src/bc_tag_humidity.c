#include "bc_tag_humidity.h"
#include "chip_hts221.h"
#include "bc_bridge.h"

static bool _bc_tag_humidity_write_register(bc_tag_humidity_t *self, uint8_t address, uint8_t value);
static bool _bc_tag_humidity_read_register(bc_tag_humidity_t *self, uint8_t address, uint8_t *value);

bool bc_tag_humidity_init(bc_tag_humidity_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;
    self->_calibration_not_read = true;

    self->is_hts221 = device_address == BC_TAG_HUMIDITY_DEVICE_ADDRESS_DEFAULT;

    self->disable_log = true;

    if (self->is_hts221)
    {
        uint8_t who_am_i;

        if (!_bc_tag_humidity_read_register(self, HTS221_WHO_AM_I, &who_am_i))
        {
            return false;
        }

        if (who_am_i != HTS221_WHO_AM_I_RESULT)
        {
            return false;
        }
    }

    if (!bc_tag_humidity_power_down(self))
    {
        return false;
    }

    self->disable_log = false;

    return true;
}

bool bc_tag_humidity_is_communication_fault(bc_tag_humidity_t *self)
{
    return self->_communication_fault;
}

bool bc_tag_humidity_get_state(bc_tag_humidity_t *self, bc_tag_humidity_state_t *state)
{
    if (self->is_hts221)
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

        if ((ctrl_reg1 & HTS221_BIT_PD) == 0)
        {
            *state = BC_TAG_HUMIDITY_STATE_POWER_DOWN;

            return true;
        }

        *state = BC_TAG_HUMIDITY_STATE_CONVERSION;


        if (!_bc_tag_humidity_read_register(self, HTS221_STATUS_REG, &status_reg))
        {
            return false;
        }

        if ((status_reg & HTS221_BIT_H_DA) != 0)
        {
            *state = BC_TAG_HUMIDITY_STATE_RESULT_READY;

            return true;
        }

        if ((ctrl_reg1 & HTS221_MASK_ODR) == HTS221_ODR_ONE_SHOT)
        {

            if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG2, &ctrl_reg2))
            {
                return false;
            }

            if ((ctrl_reg2 & HTS221_BIT_ONE_SHOT) != 0)
            {
                return true;
            }

        }

        *state = BC_TAG_HUMIDITY_STATE_POWER_UP;
    }
    else
    {
        uint8_t value;

        if (!_bc_tag_humidity_read_register(self, 0x04, &value))
        {
            return false;
        }

        *state = (value & 0x80) != 0 ? BC_TAG_HUMIDITY_STATE_RESULT_READY : BC_TAG_HUMIDITY_STATE_POWER_UP;

        return true;
    }

    return true;
}

bool bc_tag_humidity_load_calibration(bc_tag_humidity_t *self)
{
    uint8_t i;
    uint8_t calibration[16];
    int16_t h1_rh;
    int16_t h1_t0_out;


    for (i = 0; i < 16; i++)
    {
        if (!_bc_tag_humidity_read_register(self, HTS221_CALIB_OFFSET + i, &calibration[i]))
        {
            return false;
        }
    }

    self->h0_rh = (int16_t) calibration[0];
    self->h0_rh >>= 1;
    h1_rh = (int16_t) calibration[1];
    h1_rh >>= 1;

    self->h0_t0_out = (int16_t) calibration[6];
    self->h0_t0_out |= ((int16_t) calibration[7]) << 8;

    h1_t0_out = (int16_t) calibration[10];
    h1_t0_out |= ((int16_t) calibration[11]) << 8;

    if ((h1_t0_out - self->h0_t0_out) == 0)
    {
        return false;
    }

    self->h_grad = (float) (h1_rh - self->h0_rh) / (float) (h1_t0_out - self->h0_t0_out);

    uint16_t t0_degC = (int16_t) calibration[2];
    t0_degC |= (int16_t) (0x03 & calibration[5]) << 8;
    t0_degC >>= 3; // /= 8.0

    uint16_t t1_degC = (int16_t) calibration[3];
    t1_degC |= (int16_t) (0x0C & calibration[5]) << 6;
    t1_degC >>= 3;

    self->_calibration_not_read = false;

    return true;
}

bool bc_tag_humidity_power_up(bc_tag_humidity_t *self)
{
    if (self->is_hts221)
    {
        uint8_t ctrl_reg1;

        if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG1, &ctrl_reg1))
        {
            return false;
        }

        ctrl_reg1 |= HTS221_BIT_PD;

        if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, ctrl_reg1))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_power_down(bc_tag_humidity_t *self)
{
    if (self->is_hts221)
    {
        uint8_t ctrl_reg1;

        if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG1, &ctrl_reg1))
        {
            return false;
        }

        ctrl_reg1 &= ~HTS221_BIT_PD;

        if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, ctrl_reg1))
        {
            return false;
        }
    }
    else
    {
        if (!_bc_tag_humidity_write_register(self, 0x0e, 0x80))
        {
            return false;
        }
    }

    return true;
}

bool bc_tag_humidity_one_shot_conversion(bc_tag_humidity_t *self)
{
    if (self->is_hts221)
    {
        if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, HTS221_BIT_PD | HTS221_BIT_BDU))
        {
            return false;
        }

        if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG2, HTS221_BIT_ONE_SHOT))
        {
            return false;
        }
    }
    else
    {
        if (!_bc_tag_humidity_write_register(self, 0x0f, 0x05))
        {
            return false;
        }
    }

    return true;
}

bool bc_tag_humidity_continuous_conversion(bc_tag_humidity_t *self)
{
    if (self->is_hts221)
    {
        uint8_t ctrl_reg1;

        if (!_bc_tag_humidity_read_register(self, HTS221_CTRL_REG1, &ctrl_reg1))
        {
            return false;
        }

        ctrl_reg1 &= ~HTS221_MASK_ODR;
        ctrl_reg1 |= HTS221_ODR_1hz;

        if (!_bc_tag_humidity_write_register(self, HTS221_CTRL_REG1, ctrl_reg1))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool bc_tag_humidity_get_relative_humidity(bc_tag_humidity_t *self, float *humidity)
{

    int16_t h_out;
    uint8_t humidity_out_lsb;
    uint8_t humidity_out_msb;

    if (self->is_hts221)
    {
        if (!_bc_tag_humidity_read_register(self, HTS221_HUMIDITY_OUT_L, &humidity_out_lsb))
        {
            return false;
        }

        if (!_bc_tag_humidity_read_register(self, HTS221_HUMIDITY_OUT_H, &humidity_out_msb))
        {
            return false;
        }
    }
    else
    {
        if (!_bc_tag_humidity_read_register(self, 0x02, &humidity_out_lsb))
        {
            return false;
        }

        if (!_bc_tag_humidity_read_register(self, 0x03, &humidity_out_msb))
        {
            return false;
        }
    }

    h_out = (int16_t) humidity_out_lsb;
    h_out |= ((int16_t) humidity_out_msb) << 8;

    if (self->is_hts221)
    {
        *humidity = self->h0_rh + ((h_out - self->h0_t0_out) * self->h_grad);
    }
    else
    {
        *humidity = (((float) h_out) / 65536.f) * 100.;
    }

    if (*humidity >= 100.f)
    {
        *humidity = 100.f;
    }
    else if (*humidity < 0)
    {
        return false;
    }

    return true;
}

static bool _bc_tag_humidity_write_register(bc_tag_humidity_t *self, uint8_t address, uint8_t value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];
    buffer[0] = value;

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

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

static bool _bc_tag_humidity_read_register(bc_tag_humidity_t *self, uint8_t address, uint8_t *value)
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
