/**
 * 16-Bit I2C I/O Expander
 */

#ifndef _BC_I2C_pca9535_H
#define _BC_I2C_pca9535_H

#include "bc_common.h"
#include "bc_i2c.h"

typedef enum
{
    BC_I2C_pca9535_OUTPUT = 0,
    BC_I2C_pca9535_INPUT = 1

} bc_i2c_pca9535_mode_t;

typedef enum
{
    BC_I2C_pca9535_LOW = 0,
    BC_I2C_pca9535_HIGH = 1

} bc_i2c_pca9535_value_t;

typedef enum
{
    BC_I2C_pca9535_PORT0 = 0,
    BC_I2C_pca9535_PORT1 = 1,

} bc_i2c_pca9535_port_t;

typedef enum
{
    BC_I2C_pca9535_PIN0 = 0,
    BC_I2C_pca9535_PIN1 = 1,
    BC_I2C_pca9535_PIN2 = 2,
    BC_I2C_pca9535_PIN3 = 3,
    BC_I2C_pca9535_PIN4 = 4,
    BC_I2C_pca9535_PIN5 = 5,
    BC_I2C_pca9535_PIN6 = 6,
    BC_I2C_pca9535_PIN7 = 7,

} bc_i2c_pca9535_pin_t;

#define BC_I2C_pca9535_ALL_OUTPUT 0x00
#define BC_I2C_pca9535_ALL_INPUT 0xff

typedef struct
{
    bc_i2c_interface_t *_interface;
    uint8_t _device_address;
    bool _communication_fault;

} bc_i2c_pca9535_t;

bool bc_ic2_pca9535_init(bc_i2c_pca9535_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_ic2_pca9535_read_pins(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t *pins);
bool bc_ic2_pca9535_write_pins(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t pins);

//bool bc_ic2_pca9535_read_pin(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_value_t *key);
//bool bc_ic2_pca9535_write_pin(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_value_t key);

bool bc_ic2_pca9535_get_modes(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t *modes);
bool bc_ic2_pca9535_set_modes(bc_i2c_pca9535_t *self, bc_i2c_pca9535_port_t port, uint8_t modes);

//bool bc_ic2_pca9535_get_mode(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_mode_t *mode);
//bool bc_ic2_pca9535_set_mode(bc_i2c_pca9535_t *self, bc_i2c_pca9535_pin_t pin, bc_i2c_pca9535_mode_t mode);


#endif /* _BC_I2C_pca9535_H */
