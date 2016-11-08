#include "bc_i2c_ssd1306.h"
#include "bc_bridge.h"

#define BC_I2C_SSD1306_I2C_ADDRESS  0x3C
#define BC_I2C_SSD1306_SETCONTRAST  0x81
#define BC_I2C_SSD1306_DISPLAYALLON_RESUME  0xA4
#define BC_I2C_SSD1306_DISPLAYALLON  0xA5
#define BC_I2C_SSD1306_NORMALDISPLAY  0xA6
#define BC_I2C_SSD1306_INVERTDISPLAY  0xA7
#define BC_I2C_SSD1306_DISPLAYOFF  0xAE
#define BC_I2C_SSD1306_DISPLAYON  0xAF
#define BC_I2C_SSD1306_SETDISPLAYOFFSET  0xD3
#define BC_I2C_SSD1306_SETCOMPINS  0xDA
#define BC_I2C_SSD1306_SETVCOMDETECT  0xDB
#define BC_I2C_SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define BC_I2C_SSD1306_SETPRECHARGE  0xD9
#define BC_I2C_SSD1306_SETMULTIPLEX  0xA8
#define BC_I2C_SSD1306_SETLOWCOLUMN  0x00
#define BC_I2C_SSD1306_SETHIGHCOLUMN  0x10
#define BC_I2C_SSD1306_SETSTARTLINE  0x40
#define BC_I2C_SSD1306_MEMORYMODE  0x20
#define BC_I2C_SSD1306_COLUMNADDR  0x21
#define BC_I2C_SSD1306_PAGEADDR  0x22
#define BC_I2C_SSD1306_COMSCANINC  0xC0
#define BC_I2C_SSD1306_COMSCANDEC  0xC8
#define BC_I2C_SSD1306_SEGREMAP  0xA0
#define BC_I2C_SSD1306_CHARGEPUMP  0x8D
#define BC_I2C_SSD1306_EXTERNALVCC  0x1
#define BC_I2C_SSD1306_SWITCHCAPVCC  0x2

#define BC_I2C_SSD1306_ACTIVATE_SCROLL  0x2F
#define BC_I2C_SSD1306_DEACTIVATE_SCROLL  0x2E
#define BC_I2C_SSD1306_SET_VERTICAL_SCROLL_AREA  0xA3
#define BC_I2C_SSD1306_RIGHT_HORIZONTAL_SCROLL  0x26
#define BC_I2C_SSD1306_LEFT_HORIZONTAL_SCROLL  0x27
#define BC_I2C_SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL  0x29
#define BC_I2C_SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A

static bool _bc_ic2_ssd1306_command(bc_i2c_ssd1306_t *self, uint8_t command);
static bool _bc_ic2_ssd1306_send_data(bc_i2c_ssd1306_t *self, uint8_t *buffer, uint8_t length);

bool bc_ic2_ssd1306_init(bc_i2c_ssd1306_t *self, bc_i2c_interface_t *interface, uint8_t device_address)
{
    memset(self, 0, sizeof(*self));

    self->_interface = interface;
    self->_device_address = device_address;
    self->size = BC_I2C_SSD1306_128_64;

    self->width = 128;
    self->height = 64;
    self->_pages = self->height/8;

    self->length = self->width*self->_pages;

    self->disable_log = true;

    if (_bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_DISPLAYOFF) && // 0xAE
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETDISPLAYCLOCKDIV) && // 0xD5
        _bc_ic2_ssd1306_command(self, 0x80) && // the suggested ratio 0x80
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETMULTIPLEX) && // 0xA8
        _bc_ic2_ssd1306_command(self, 0x3F) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETDISPLAYOFFSET) && // 0xD3
        _bc_ic2_ssd1306_command(self, 0x0) && // no offset
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETSTARTLINE | 0x0)&& // line #0
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_CHARGEPUMP) && // 0x8D
        _bc_ic2_ssd1306_command(self, 0x14) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_MEMORYMODE) && // 0x20
        _bc_ic2_ssd1306_command(self, 0x00) && // Horizontal addressing mode
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SEGREMAP | 0x1) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_COMSCANDEC) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETCOMPINS) && // 0xDA
        _bc_ic2_ssd1306_command(self, 0x12) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETCONTRAST) && // 0x81
        _bc_ic2_ssd1306_command(self, 0xCF) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETPRECHARGE) && // 0xd9
        _bc_ic2_ssd1306_command(self, 0xF1) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_SETVCOMDETECT) && // 0xDB
        _bc_ic2_ssd1306_command(self, 0x40) &&
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_DISPLAYALLON_RESUME) && // 0xA4
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_NORMALDISPLAY) && // 0xA6
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_DISPLAYON) ) // Turn on the display.
    {

        self->disable_log = false;

        self->buffer = malloc(self->length*sizeof(uint8_t));

        return  true;
    }

    return false;
}

void bc_ic2_ssd1306_destroy(bc_i2c_ssd1306_t *self)
{
    free(self->buffer);
}

bool bc_ic2_ssd1306_display(bc_i2c_ssd1306_t *self)
{
    int i;

    if (_bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_COLUMNADDR) &&
    _bc_ic2_ssd1306_command(self, 0) &&        // Column start address. (0 = reset)
    _bc_ic2_ssd1306_command(self, self->width-1 ) &&    // Column end address.
    _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_PAGEADDR) &&
        _bc_ic2_ssd1306_command(self, 0)  &&        // Page start address. (0 = reset)
    _bc_ic2_ssd1306_command(self, self->_pages-1) ) // Page end address.
    {

        for (i=0; i<self->length; i+=8)
        {
            if (!_bc_ic2_ssd1306_send_data(self, self->buffer+i, 8 ))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool bc_ic2_ssd1306_display_page(bc_i2c_ssd1306_t *self, uint8_t page)
{
    int i;

    if (_bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_COLUMNADDR) &&
        _bc_ic2_ssd1306_command(self, 0) &&       // Column start address. (0 = reset)
        _bc_ic2_ssd1306_command(self, self->width-1 )  &&    // Column end address.
        _bc_ic2_ssd1306_command(self, BC_I2C_SSD1306_PAGEADDR)  &&
        _bc_ic2_ssd1306_command(self, page)  &&       // Page start address. (0 = reset)
        _bc_ic2_ssd1306_command(self, page+1) ) // Page end address.
    {
        for (i=page*self->width; i<(page+1)*self->width; i+=8)
        {
            if (!_bc_ic2_ssd1306_send_data(self, self->buffer+i, 8 ))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

static bool _bc_ic2_ssd1306_command(bc_i2c_ssd1306_t *self, uint8_t command)
{
    bc_i2c_transfer_t transfer;

    uint8_t buffer[1];
    buffer[0] = (uint8_t) command;

    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = 0x00;
    transfer.length = 1;

#ifdef BRIDGE
    transfer.disable_log = self->disable_log;

    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_write(self->_interface->bridge, &transfer))
    {
        return false;
    }
#else

    bool communication_fault;

    if (!self->_interface->write(&transfer, &communication_fault))
    {
        return false;
    }
#endif

    return true;
}

static bool _bc_ic2_ssd1306_send_data(bc_i2c_ssd1306_t *self, uint8_t *buffer, uint8_t length)
{
    bc_i2c_transfer_t transfer;
    bc_i2c_transfer_init(&transfer);

    transfer.device_address = self->_device_address;
    transfer.buffer = buffer;
    transfer.address = 0x40;
    transfer.length = length;

#ifdef BRIDGE
    transfer.disable_log = self->disable_log;

    transfer.channel = self->_interface->channel;
    if (!bc_bridge_i2c_write(self->_interface->bridge, &transfer))
    {
        return false;
    }
#else

    bool communication_fault;

    if (!self->_interface->write(&transfer, &communication_fault))
    {
        return false;
    }
#endif

    return true;
}