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
     BC_I2C_TCA9534A_PIN0 = 0x01,
     BC_I2C_TCA9534A_PIN1 = 0x02,
     BC_I2C_TCA9534A_PIN2 = 0x04,
     BC_I2C_TCA9534A_PIN3 = 0x08,
     BC_I2C_TCA9534A_PIN4 = 0x10,
     BC_I2C_TCA9534A_PIN5 = 0x20,
     BC_I2C_TCA9534A_PIN6 = 0x40,
     BC_I2C_TCA9534A_PIN7 = 0x80

} bc_i2c_tca9534a_pin_t;

typedef struct
{
	bc_tag_interface_t *_interface;
	uint8_t _device_address;
	bool _communication_fault;

} bc_i2c_tca9534a_t;

bool br_ic2_tca9534a_init(bc_i2c_tca9534a_t *self, bc_tag_interface_t *interface, uint8_t device_address);
bool bc_ic2_tca9534a_read_pins(bc_i2c_tca9534a_t *self, uint8_t *pins);
bool bc_ic2_tca9534a_read_pin(bc_i2c_tca9534a_t *self, bc_i2c_tca9534a_pin_t pin, bc_i2c_tca9534a_value_t *value);



#endif /* _BC_I2C_TCA9534A_H */