#ifndef _BC_I2C_TCA9534A_H
#define _BC_I2C_TCA9534A_H

#include "bc_common.h"
#include "bc_i2c.h"

#define BC_I2C_TCA9534A_DIRECTION_ALL_OUTPUT 0x00
#define BC_I2C_TCA9534A_DIRECTION_ALL_INPUT 0xFF

typedef enum
{
	BC_I2C_TCA9534A_PIN_P0 = 0,
	BC_I2C_TCA9534A_PIN_P1 = 1,
	BC_I2C_TCA9534A_PIN_P2 = 2,
	BC_I2C_TCA9534A_PIN_P3 = 3,
	BC_I2C_TCA9534A_PIN_P4 = 4,
	BC_I2C_TCA9534A_PIN_P5 = 5,
	BC_I2C_TCA9534A_PIN_P6 = 6,
	BC_I2C_TCA9534A_PIN_P7 = 7

} bc_i2c_tca9534a_pin_t;

typedef enum
{
     BC_I2C_TCA9534A_DIRECTION_OUTPUT = 0,
     BC_I2C_TCA9534A_DIRECTION_INPUT = 1

} bc_i2c_tca9534a_direction_t;

typedef enum
{
     BC_I2C_TCA9534A_VALUE_LOW = 0,
     BC_I2C_TCA9534A_VALUE_HIGH = 1

} bc_i2c_tca9534a_value_t;

typedef struct
{
	bc_i2c_interface_t *_interface;
	uint8_t _device_address;
	bool _communication_fault;

} bc_i2c_tca9534a_t;

bool bc_i2c_tca9534a_init(bc_i2c_tca9534a_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_i2c_tca9534a_read_port(bc_i2c_tca9534a_t *self, uint8_t *value);
bool bc_i2c_tca9534a_write_port(bc_i2c_tca9534a_t *self, uint8_t value);
bool bc_i2c_tca9534a_read_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t *value);
bool bc_i2c_tca9534a_write_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t value);
bool bc_i2c_tca9534a_get_port_direction(bc_i2c_tca9534a_t *self, uint8_t *direction);
bool bc_i2c_tca9534a_set_port_direction(bc_i2c_tca9534a_t *self, uint8_t direction);
bool bc_i2c_tca9534a_get_pin_direction(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin,
                                       bc_i2c_tca9534a_direction_t *direction);
bool bc_i2c_tca9534a_set_pin_direction(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin,
                                       bc_i2c_tca9534a_direction_t direction);

#endif /* _BC_I2C_TCA9534A_H */
