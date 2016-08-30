#include <bc/i2c/sc16is740.h>

#define BC_I2C_SC16IS740_DEBUG 1
#define BC_I2C_SC16IS740_CRYSTCAL_FREQ (13560000UL)

static bool _bc_ic2_sc16is740_write_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t value);
static bool _bc_ic2_sc16is740_read_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t *value);
static bool _bc_ic2_sc16is740_set_default(bc_i2c_sc16is740_t *self);

bool bc_ic2_sc16is740_init(bc_i2c_sc16is740_t *self, bc_tag_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;

    return _bc_ic2_sc16is740_set_default(self);
}

bool bc_ic2_sc16is740_reset_device(bc_i2c_sc16is740_t *self)
{
    uint8_t register_iocontrol;
    if (!_bc_ic2_sc16is740_read_register(self, 0x0E, &register_iocontrol)){
        return false;
    }
    register_iocontrol |= 0x08;
    if (!_bc_ic2_sc16is740_write_register(self, 0x0E, register_iocontrol)){
        return false;
    }
    return true;
}

bool bc_ic2_sc16is740_reset_fifo(bc_i2c_sc16is740_t *self, bc_i2c_sc16is740_fifo_t fifo)
{
    uint8_t register_fcr;
    if (!_bc_ic2_sc16is740_read_register(self, 0x02, &register_fcr)){
        return false;
    }
    register_fcr |= fifo;
    if (!_bc_ic2_sc16is740_write_register(self, 0x02, register_fcr)){
        return false;
    }
    return true;
}

bool bc_ic2_sc16is740_available(bc_i2c_sc16is740_t *self)
{
    uint8_t register_rxlvl;
    if (!_bc_ic2_sc16is740_read_register(self, 0x09, &register_rxlvl)){
        return false;
    }
    return register_rxlvl!=0;
}

bool bc_ic2_sc16is740_read(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length, bc_tick_t timeout)
{
    uint8_t register_rhr;
    uint8_t i;
    bc_tick_t stop = bc_tick_get() + timeout;
    uint8_t register_rxlvl;

    while ( bc_tick_get() < stop )
    {
        if (!_bc_ic2_sc16is740_read_register(self, 0x09, &register_rxlvl))
        {
            return false;
        }
        if (register_rxlvl == 0)
        {
            if (!_bc_ic2_sc16is740_read_register(self, 0x00, &register_rhr)){
                return false;
            }
            data[i++] = register_rhr;
        }
        if (i==length)
        {
            return true;
        }
    }

    return false;
}

bool bc_ic2_sc16is740_available_write(bc_i2c_sc16is740_t *self)
{
    uint8_t register_txlvl;
    if (!_bc_ic2_sc16is740_read_register(self, 0x08, &register_txlvl)){
        return false;
    }
    fprintf(stderr, "register txlvl %x \n", register_txlvl);
    return register_txlvl==0;
}

bool bc_ic2_sc16is740_write(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length)
{
    uint8_t i;
    for(i=0; i<length; i++){
        //bc_ic2_sc16is740_available_write(self);
        printf("bc_ic2_sc16is740_write %d %x \n", i, data[i]);
        if (!_bc_ic2_sc16is740_write_register(self, 0x00, data[i])){
            printf("write error i = %d \n", i);
            return false;
        }
    }

    return true;
}


static bool _bc_ic2_sc16is740_set_default(bc_i2c_sc16is740_t *self)
{
    uint32_t baudrate = 9600;

    uint8_t register_lcr;
    uint8_t register_mcr;
    uint8_t register_efr;
    uint8_t register_fcr;

    uint8_t prescaler;
    uint16_t divisor;

    if (!_bc_ic2_sc16is740_read_register(self, 0X04, &register_mcr)){ //MCR
        return false;
    }
    if ((register_mcr & 0x80) == 0x00 )
    {
        prescaler = 1;
    } else {
        prescaler = 4;
    }

    divisor = (BC_I2C_SC16IS740_CRYSTCAL_FREQ/prescaler)/(baudrate*16);


    register_lcr = 0x80; //switch to access Special register
    if (!_bc_ic2_sc16is740_write_register(self, 0x03, register_lcr)){
        return false;
    }

    if (!_bc_ic2_sc16is740_write_register(self, 0x00, (uint8_t)divisor )){ //DLL
        return false;
    }

    if (!_bc_ic2_sc16is740_write_register(self, 0x01, (uint8_t)(divisor>>8) )){ //DLH
        return false;
    }

    register_lcr = 0xBF; //switch to access Enhanced register
    if (!_bc_ic2_sc16is740_write_register(self, 0x03, register_lcr)){
        return false;
    }
    //no transmit flow control
    if (!_bc_ic2_sc16is740_read_register(self, 0x02, &register_efr)){
        return false;
    }
    register_efr &= 0xf0;
    if (!_bc_ic2_sc16is740_write_register(self, 0x02, register_efr)){
        return false;
    }

    register_lcr = 0b10000111;
    //divisor latch enabled
    //no break
    //no parity
    //2 stop bits
    //8 data bits

    if (!_bc_ic2_sc16is740_write_register(self, 0x03, register_lcr)){
        return false;
    }

    //fifo enabled
    if (!_bc_ic2_sc16is740_read_register(self, 0x02, &register_fcr)){
        return false;
    }
    register_fcr |= 0x01;
    if (!_bc_ic2_sc16is740_write_register(self, 0x02, register_fcr)){
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
    buffer[0] = (uint8_t) value;

    bc_tag_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address;
    transfer.length = 1;

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