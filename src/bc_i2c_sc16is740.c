#include "bc_i2c_sc16is740.h"
#include "bc_log.h"
#include "bc_bridge.h"

#define BC_I2C_SC16IS740_CRYSTCAL_FREQ (13560000UL)
#define BC_I2C_SC16IS740_BAUDRATE       9600

#define BC_I2C_SC16IS740_REG_RHR                 0x00
#define BC_I2C_SC16IS740_REG_THR                 0x00
#define BC_I2C_SC16IS740_REG_IER                 0x01
#define BC_I2C_SC16IS740_REG_FCR                 0x02
#define BC_I2C_SC16IS740_REG_LCR                 0x03
#define BC_I2C_SC16IS740_REG_MCR                 0x04
#define BC_I2C_SC16IS740_BIT_FIFO_ENABLE         0x01
#define BC_I2C_SC16IS740_REG_SPR                 0x07
#define BC_I2C_SC16IS740_REG_TXLVL               0x08
#define BC_I2C_SC16IS740_REG_RXLVL               0x09
#define BC_I2C_SC16IS740_REG_IOCONTROL           0x0E
#define BC_I2C_SC16IS740_BIT_UART_SOFTWARE_RESET 0x08

#define BC_I2C_SC16IS740_LCR_SPECIAL_REGISTER    0x80
#define BC_I2C_SC16IS740_SPECIAL_REG_DLL         0x00
#define BC_I2C_SC16IS740_SPECIAL_REG_DLH         0x01

#define BC_I2C_SC16IS740_LCR_SPECIAL_ENHANCED_REGISTER  0xBF
#define BC_I2C_SC16IS740_ENHANCED_REG_EFR        0x02


static bool _bc_ic2_sc16is740_write_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t value);
static bool _bc_ic2_sc16is740_read_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t *value);
static bool _bc_ic2_sc16is740_set_default(bc_i2c_sc16is740_t *self);

bool bc_ic2_sc16is740_init(bc_i2c_sc16is740_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->_communication_fault = true;

    if (!_bc_ic2_sc16is740_set_default(self))
    {
        bc_log_error("bc_ic2_sc16is740_init: call failed: _bc_ic2_sc16is740_set_default");
        return false;
    }

    return true;
}

bool bc_ic2_sc16is740_reset_device(bc_i2c_sc16is740_t *self)
{
    uint8_t register_iocontrol;
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_IOCONTROL, &register_iocontrol))
    {
        return false;
    }
    register_iocontrol |= BC_I2C_SC16IS740_BIT_UART_SOFTWARE_RESET;
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_IOCONTROL, register_iocontrol))
    {
        return false;
    }
    return true;
}

bool bc_ic2_sc16is740_reset_fifo(bc_i2c_sc16is740_t *self, bc_i2c_sc16is740_fifo_t fifo)
{
    uint8_t register_fcr;
    register_fcr = fifo | BC_I2C_SC16IS740_BIT_FIFO_ENABLE;
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_FCR, register_fcr))
    {
        return false;
    }
    return true;
}

bool bc_ic2_sc16is740_available(bc_i2c_sc16is740_t *self)
{
    uint8_t register_rxlvl;
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_RXLVL, &register_rxlvl))
    {
        return false;
    }
    return register_rxlvl != 0;
}

bool bc_ic2_sc16is740_read(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length, bc_tick_t timeout)
{
    uint8_t register_rhr;
    uint8_t read_length = 0;
    bc_tick_t stop = bc_tick_get() + timeout;
    uint8_t register_rxlvl;

    bc_log_debug("bc_ic2_sc16is740_read: length %d bytes, timeout %d", length, timeout);

    bc_i2c_transfer_t transfer;
    bc_i2c_transfer_init(&transfer);
    transfer.device_address = self->_device_address;
    transfer.address = BC_I2C_SC16IS740_REG_RHR;


    while (bc_tick_get() < stop)
    {
        if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_RXLVL, &register_rxlvl))
        {
            bc_log_error("bc_ic2_sc16is740_read: call failed: _bc_ic2_sc16is740_read_register RXLVL");
            return false;
        }

        if (register_rxlvl != 0)
        {

            bc_log_debug("register_rhr length: %d register_rhr: %d read_length: %d", length, register_rxlvl, read_length);

            transfer.buffer = data + read_length;
            transfer.length = length - read_length;
            if (transfer.length > register_rxlvl)
            {
                transfer.length = register_rxlvl;
            }

            if (transfer.length < 1)
            {
                return false;
            }

#ifdef BRIDGE
            self->_communication_fault = true;
            transfer.channel = self->_interface->channel;
            if (!bc_bridge_i2c_read(self->_interface->bridge, &transfer))
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

            read_length += transfer.length;

            //data[i++] = register_rhr;
            stop = bc_tick_get() + timeout;
        }
        else
        {
            bc_os_task_sleep(1);
        }

        if (read_length == length)
        {
            bc_log_dump(data, length, "bc_ic2_sc16is740_read: read %d bytes", length);
            return true;
        }
    }

    bc_log_error("bc_ic2_sc16is740_read: timeout");

    return false;
}

/**
 * spaces_available max 64
 */
bool bc_ic2_sc16is740_get_txfifo_spaces_available(bc_i2c_sc16is740_t *self, uint8_t *txfifo_spaces_available)
{
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_TXLVL, txfifo_spaces_available))
    {
        return false;
    }
    return true;
}

bool bc_ic2_sc16is740_write(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length)
{

    uint8_t txfifo_spaces_available;

    if (length > 64)
    {
        bc_log_error("bc_ic2_sc16is740_write: length is too big");
        return false;
    }

    bc_log_dump(data, length, "bc_ic2_sc16is740_write: length %d bytes", length);

    do
    {
        bc_ic2_sc16is740_get_txfifo_spaces_available(self, &txfifo_spaces_available);
    } while (txfifo_spaces_available < length);

    bc_i2c_transfer_t transfer;
    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = data;
    transfer.address = BC_I2C_SC16IS740_REG_THR;
    transfer.length = length;

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


static bool _bc_ic2_sc16is740_set_default(bc_i2c_sc16is740_t *self)
{
    uint32_t baudrate = BC_I2C_SC16IS740_BAUDRATE;

    uint8_t register_lcr;
    uint8_t register_mcr;
    uint8_t register_efr;
    uint8_t register_fcr;
    uint8_t register_ier;
    uint8_t register_spr;

    uint8_t prescaler;
    uint16_t divisor;

    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_MCR, &register_mcr))
    { //MCR
        return false;
    }
    if ((register_mcr & 0x80) == 0x00)
    {
        prescaler = 1;
    }
    else
    {
        prescaler = 4;
    }

    divisor = (BC_I2C_SC16IS740_CRYSTCAL_FREQ / prescaler) / (baudrate * 16);

    //baudrate
    register_lcr = BC_I2C_SC16IS740_LCR_SPECIAL_REGISTER; //switch to access Special register
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_LCR, register_lcr))
    {
        return false;
    }
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_SPECIAL_REG_DLL, (uint8_t) divisor))
    { //DLL
        return false;
    }
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_SPECIAL_REG_DLH, (uint8_t) (divisor >> 8)))
    { //DLH
        return false;
    }

    //no transmit flow control
    register_lcr = BC_I2C_SC16IS740_LCR_SPECIAL_ENHANCED_REGISTER; //switch to access Enhanced register
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_LCR, register_lcr))
    {
        return false;
    }
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_ENHANCED_REG_EFR, &register_efr))
    {
        return false;
    }
    register_efr &= 0xf0;
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_ENHANCED_REG_EFR, register_efr))
    {
        return false;
    }

    register_lcr = 0x07; //General register set
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_LCR, register_lcr))
    {
        return false;
    }

    //fifo enabled
    register_fcr = BC_I2C_SC16IS740_BIT_FIFO_ENABLE;
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_FCR, register_fcr))
    {
        return false;
    }
    bc_os_task_sleep(1);

    //Polled mode operation
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_IER, &register_ier))
    {
        return false;
    }
    register_ier &= 0xf0;
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_IER, register_ier))
    {
        return false;
    }


    register_lcr = 0x07;

    //no break
    //no parity
    //2 stop bits
    //8 data bits

    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_LCR, register_lcr))
    {
        return false;
    }

    bc_log_debug("_bc_ic2_sc16is740_set_default: baudrate error %0.2f %%, prescaler %d, divisor %d ",
                 ((float) ((BC_I2C_SC16IS740_CRYSTCAL_FREQ / prescaler) / (16 * divisor)) - baudrate) * 100 / baudrate,
                 prescaler, divisor);

    //TEST
    if (!_bc_ic2_sc16is740_write_register(self, BC_I2C_SC16IS740_REG_SPR, 0x48))
    {
        return false;
    }
    if (!_bc_ic2_sc16is740_read_register(self, BC_I2C_SC16IS740_REG_SPR, &register_spr))
    { //MCR
        return false;
    }

    return register_spr == 0x48;
}


static bool _bc_ic2_sc16is740_write_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];
    buffer[0] = (uint8_t) value;

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address << 3;
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

static bool _bc_ic2_sc16is740_read_register(bc_i2c_sc16is740_t *self, uint8_t address, uint8_t *value)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = address << 3;
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
