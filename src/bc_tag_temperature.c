#include "bc_tag_temperature.h"
#include "chip_tmp112.h"

static bool _bc_tag_temperature_write_register(bc_tag_temperature_t *self, uint8_t address, uint16_t value);
static bool _bc_tag_temperature_read_register(bc_tag_temperature_t *self, uint8_t address, uint16_t *value);

bool bc_tag_temperature_init(bc_tag_temperature_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;
    self->_configuration = TMP112_BIT_R1 | TMP112_BIT_R0 | TMP112_BIT_SD | TMP112_BIT_CR1;

    if (!bc_tag_temperature_power_down(self))
    {
        return false;
    }

    return true;
}

bool bc_tag_temperature_is_communication_fault(bc_tag_temperature_t *self)
{
    return self->_communication_fault;
}

bool bc_tag_temperature_get_state(bc_tag_temperature_t *self, bc_tag_temperature_state_t *state)
{
    uint16_t configuration;

    if (!_bc_tag_temperature_read_register(self, TMP112_REG_CONFIGURATION, &configuration))
    {
        return false;
    }

    if ((configuration & TMP112_BIT_SD) == 0)
    {
        *state = BC_TAG_TEMPERATURE_STATE_CONVERSION;
    }
    else
    {
        *state = BC_TAG_TEMPERATURE_STATE_POWER_DOWN;
    }

    return true;
}

bool bc_tag_temperature_power_down(bc_tag_temperature_t *self)
{
    uint16_t configuration;

    configuration = self->_configuration;
    configuration |= TMP112_BIT_SD;

    if (!_bc_tag_temperature_write_register(self, TMP112_REG_CONFIGURATION, configuration))
    {
        return false;
    }

    return true;
}

bool bc_tag_temperature_single_shot_conversion(bc_tag_temperature_t *self)
{
    uint16_t configuration;

    configuration = self->_configuration;
    configuration |= TMP112_BIT_OS;

    if (!_bc_tag_temperature_write_register(self, TMP112_REG_CONFIGURATION, configuration))
    {
        return false;
    }

    return true;
}

bool bc_tag_temperature_continuous_conversion(bc_tag_temperature_t *self)
{
    uint16_t configuration;

    configuration = self->_configuration;
    configuration &= ~TMP112_BIT_SD;

    if (!_bc_tag_temperature_write_register(self, TMP112_REG_CONFIGURATION, configuration))
    {
        return false;
    }

    return true;
}

bool bc_tag_temperature_read_temperature(bc_tag_temperature_t *self)
{
    uint16_t temperature;

    if (!_bc_tag_temperature_read_register(self, TMP112_REG_TEMPERATURE, &temperature))
    {
        return false;
    }

    self->_temperature = temperature;

    return true;
}

bool bc_tag_temperature_get_temperature_raw(bc_tag_temperature_t *self, int16_t *raw)
{
    *raw = ((int16_t) self->_temperature) >> 4;

    return true;
}

bool bc_tag_temperature_get_temperature_celsius(bc_tag_temperature_t *self, float *celsius)
{
    int16_t raw;

    if (!bc_tag_temperature_get_temperature_raw(self, &raw))
    {
        return false;
    }

    *celsius = ((float) raw) / 16.f;

    return true;
}

bool bc_tag_temperature_get_temperature_fahrenheit(bc_tag_temperature_t *self, float *fahrenheit)
{
    float celsius;

    if (!bc_tag_temperature_get_temperature_celsius(self, &celsius))
    {
        return false;
    }

    *fahrenheit = (celsius * 1.8f) + 32.f;

    return true;
}

bool bc_tag_temperature_get_temperature_kelvin(bc_tag_temperature_t *self, float *kelvin)
{
    float celsius;

    if (!bc_tag_temperature_get_temperature_celsius(self, &celsius))
    {
        return false;
    }

    *kelvin = celsius + 273.15f;

    /* This is actually a kind of joke ;-) */
    if (*kelvin < 0.f)
    {
        *kelvin = 0.f;
    }

    return true;
}

static bool _bc_tag_temperature_write_register(bc_tag_temperature_t *self, uint8_t address, uint16_t value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[2];

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 2;

    buffer[0] = (uint8_t) (value >> 8);
    buffer[1] = (uint8_t) value;

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

static bool _bc_tag_temperature_read_register(bc_tag_temperature_t *self, uint8_t address, uint16_t *value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[2];

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 2;

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

    *value = (uint16_t) buffer[1];
    *value |= ((uint16_t) buffer[0]) << 8;

    return true;
}
