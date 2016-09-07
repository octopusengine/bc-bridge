#ifndef _BC_TAG_ACCELEROMETER_H
#define _BC_TAG_ACCELEROMETER_H

#include "bc_i2c.h"

#define BC_TAG_ACCELEROMETER_ADDRESS_DEFAULT 0x18
#define BC_TAG_ACCELEROMETER_ADDRESS_ALTERNATE 0x19

typedef enum
{
	BC_TAG_ACCELEROMETER_STATE_POWER_DOWN = 0,
	BC_TAG_ACCELEROMETER_STATE_CONVERSION = 1,
	BC_TAG_ACCELEROMETER_STATE_RESULT_READY = 2

} bc_tag_accelerometer_state_t;

typedef struct
{
	int16_t x_axis;
	int16_t y_axis;
	int16_t z_axis;

} bc_tag_accelerometer_result_raw_t;

typedef struct
{
	float x_axis;
	float y_axis;
	float z_axis;

} bc_tag_accelerometer_result_g_t;

typedef struct
{
	bc_tag_interface_t *_interface;
	uint8_t _device_address;
	bool _communication_fault;
	uint8_t _out_x_l;
	uint8_t _out_x_h;
	uint8_t _out_y_l;
	uint8_t _out_y_h;
	uint8_t _out_z_l;
	uint8_t _out_z_h;

} bc_tag_accelerometer_t;

bool bc_tag_accelerometer_init(bc_tag_accelerometer_t *self, bc_tag_interface_t *interface, uint8_t device_address);
bool bc_tag_accelerometer_is_communication_fault(bc_tag_accelerometer_t *self);
bool bc_tag_accelerometer_get_state(bc_tag_accelerometer_t *self, bc_tag_accelerometer_state_t *state);
bool bc_tag_accelerometer_power_down(bc_tag_accelerometer_t *self);
bool bc_tag_accelerometer_continuous_conversion(bc_tag_accelerometer_t *self);
bool bc_tag_accelerometer_read_result(bc_tag_accelerometer_t *self);
bool bc_tag_accelerometer_get_result_raw(bc_tag_accelerometer_t *self, bc_tag_accelerometer_result_raw_t *result_raw);
bool bc_tag_accelerometer_get_result_g(bc_tag_accelerometer_t *self, bc_tag_accelerometer_result_g_t *result_g);

#endif /* _BC_TAG_ACCELEROMETER_H */
