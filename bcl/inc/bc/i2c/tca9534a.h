#ifndef _BC_I2C_TCA9534A_H
#define _BC_I2C_TCA9534A_H

#include <stdbool.h>
#include <stdint.h>
#include <bc/tag.h>

typedef enum
{
     BC_I2C_TCA9534A_OUTPUT = 0,
     BC_I2C_TCA9534A_INPUT = 1

} bc_i2c_tca9534a_mode_t;

typedef enum
{
     BC_I2C_TCA9534A_LOW = 0,
     BC_I2C_TCA9534A_HIGH = 1

} bc_i2c_tca9534a_value_t;

typedef enum
{
     BC_I2C_TCA9534A_PIN0 = 0,
     BC_I2C_TCA9534A_PIN1 = 1,
     BC_I2C_TCA9534A_PIN2 = 2,
     BC_I2C_TCA9534A_PIN3 = 3,
     BC_I2C_TCA9534A_PIN4 = 4,
     BC_I2C_TCA9534A_PIN5 = 5,
     BC_I2C_TCA9534A_PIN6 = 6,
     BC_I2C_TCA9534A_PIN7 = 7

} bc_i2c_tca9534a_pin_t;

typedef struct
{
	bc_tag_interface_t *_interface;
	uint8_t _device_address;
	bool _communication_fault;

} bc_i2c_tca9534a_t;

bool br_ic2_tca9534a_init(bc_i2c_tca9534a_t *self, bc_tag_interface_t *interface, uint8_t device_address);
bool bc_ic2_tca9534a_read_pins(bc_i2c_tca9534a_t *self, uint8_t *pins);
bool bc_ic2_tca9534a_write_pins(bc_i2c_tca9534a_t *self, uint8_t pins);
bool bc_ic2_tca9534a_read_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t *value);
bool bc_ic2_tca9534a_write_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t value);
bool bc_ic2_tca9534a_get_mode(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_mode_t *mode);
bool bc_ic2_tca9534a_set_mode(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_mode_t mode);


#endif /* _BC_I2C_TCA9534A_H */