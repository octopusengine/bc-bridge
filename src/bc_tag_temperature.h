#ifndef _BC_TAG_TEMPERATURE_H
#define _BC_TAG_TEMPERATURE_H

#include "bc_i2c.h"

#define BC_TAG_TEMPERATURE_ADDRESS_DEFAULT 0x48
#define BC_TAG_TEMPERATURE_ADDRESS_ALTERNATE 0x49

typedef struct
{
	bc_i2c_interface_t *_interface;
	uint8_t _device_address;
	bool _communication_fault;
	uint16_t _configuration;
	uint16_t _temperature;

} bc_tag_temperature_t;

typedef enum
{
	BC_TAG_TEMPERATURE_STATE_POWER_DOWN = 0,
	BC_TAG_TEMPERATURE_STATE_CONVERSION = 1

} bc_tag_temperature_state_t;

bool bc_tag_temperature_init(bc_tag_temperature_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_tag_temperature_is_communication_fault(bc_tag_temperature_t *self);
bool bc_tag_temperature_get_state(bc_tag_temperature_t *self, bc_tag_temperature_state_t *state);
bool bc_tag_temperature_power_down(bc_tag_temperature_t *self);
bool bc_tag_temperature_single_shot_conversion(bc_tag_temperature_t *self);
bool bc_tag_temperature_continuous_conversion(bc_tag_temperature_t *self);
bool bc_tag_temperature_read_temperature(bc_tag_temperature_t *self);
bool bc_tag_temperature_get_temperature_raw(bc_tag_temperature_t *self, int16_t *raw);
bool bc_tag_temperature_get_temperature_celsius(bc_tag_temperature_t *self, float *celsius);
bool bc_tag_temperature_get_temperature_fahrenheit(bc_tag_temperature_t *self, float *fahrenheit);
bool bc_tag_temperature_get_temperature_kelvin(bc_tag_temperature_t *self, float *kelvin);

#endif /* _BC_TAG_TEMPERATURE_H */
