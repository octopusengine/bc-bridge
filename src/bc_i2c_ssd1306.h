/**
 * 128 x 64 Dot Matrix OLED I2C Driver
 */

#ifndef BC_I2C_SSD1306_H
#define BC_I2C_SSD1306_H

#include "bc_common.h"
#include "bc_i2c.h"

typedef enum {
    BC_I2C_SSD1306_128_64
} bc_i2c_ssd1306_size_t;

typedef struct
{
    bc_i2c_interface_t *_interface;
    uint8_t _device_address;
    bool disable_log;

    bc_i2c_ssd1306_size_t size;
    uint8_t width;
    uint8_t height;

    uint8_t _pages;

    uint8_t *buffer;
    size_t length;

} bc_i2c_ssd1306_t;

bool bc_ic2_ssd1306_init(bc_i2c_ssd1306_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
void bc_ic2_ssd1306_destroy(bc_i2c_ssd1306_t *self);
bool bc_ic2_ssd1306_display(bc_i2c_ssd1306_t *self);
bool bc_ic2_ssd1306_display_page(bc_i2c_ssd1306_t *self, uint8_t page);

#endif //BC_I2C_SSD1306_H
