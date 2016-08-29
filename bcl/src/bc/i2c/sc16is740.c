#include <bc/i2c/sc16is740.h>

#define BC_I2C_SC16IS740_DEBUG 1

static bool _bc_ic2_sc16is740_write_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t value);
static bool _bc_ic2_sc16is740_read_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t *value);

#define BC_I2C_SC16IS740_CRYSTCAL_FREQ (13560000UL)


bool bc_ic2_sc16is740_init(bc_i2c_sc16is740_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;
    return true;
}

bool bc_ic2_sc16is740_set_baudrate(bc_i2c_sc16is740_t *self, uint32_t baudrate)
{
    uint8_t reg_value;
    uint8_t prescaler;
    uint16_t divisor;

    if (!_bc_ic2_sc16is740_read_register(self, 0X04, &reg_value)){ //MCR
        return false;
    }
    if ((reg_value & 0x80) == 0x00 )
    {
        prescaler = 1;
    } else {
        prescaler = 4;
    }

    divisor = (BC_I2C_SC16IS740_CRYSTCAL_FREQ/prescaler)/(baudrate*16);

    if (!_bc_ic2_sc16is740_read_register(self, 0x03, &reg_value)){ //LCR
        return false;
    }
    reg_value |= 0x80;
    if (!_bc_ic2_sc16is740_write_register(self, 0x03, &reg_value)){ //LCR
        return false;
    }

    if (!_bc_ic2_sc16is740_write_register(self, 0x00, (uint8_t)divisor )){ //DLL
        return false;
    }

    if (!_bc_ic2_sc16is740_write_register(self, 0x01, (uint8_t)(divisor>>8) )){ //DLH
        return false;
    }

    reg_value &= 0x7F;
    if (!_bc_ic2_sc16is740_write_register(self, 0x03, &reg_value)){ //LCR
        return false;
    }

#ifdef BC_I2C_SC16IS740_DEBUG
    uint32_t actual_baudrate = (BC_I2C_SC16IS740_CRYSTCAL_FREQ/prescaler)/(16*divisor);
    fprintf(stderr, "prescaler %d \n", prescaler);
    fprintf(stderr, "divisor %d \n", divisor);
    fprintf(stderr, "baudrate error %0.2f %% \n",((float)actual_baudrate-baudrate)*100/baudrate );
#endif

    return  true;
}


static bool _bc_ic2_sc16is740_write_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t value)
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

static bool _bc_ic2_sc16is740_read_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t *value)
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

    *value = buffer[0];

    return true;
}