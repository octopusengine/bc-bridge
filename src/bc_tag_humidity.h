#ifndef _BC_TAG_HUMIDITY_H
#define _BC_TAG_HUMIDITY_H

#include "bc_i2c.h"

#define BC_TAG_HUMIDITY_DEVICE_ADDRESS 0x5F

typedef enum
{
	BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ = 0,
	BC_TAG_HUMIDITY_STATE_POWER_DOWN = 1,
	BC_TAG_HUMIDITY_STATE_POWER_UP = 2,
	BC_TAG_HUMIDITY_STATE_CONVERSION = 3,
	BC_TAG_HUMIDITY_STATE_RESULT_READY = 4

} bc_tag_humidity_state_t;

typedef struct
{
	bc_i2c_interface_t *_interface;
	bool _communication_fault;
	bool _calibration_not_read;
	uint8_t _calibration[16];
	uint8_t _humidity_out_lsb;
	uint8_t _humidity_out_msb;

} bc_tag_humidity_t;

bool bc_tag_humidity_init(bc_tag_humidity_t *self, bc_i2c_interface_t *interface);
bool bc_tag_humidity_is_communication_fault(bc_tag_humidity_t *self);
bool bc_tag_humidity_get_state(bc_tag_humidity_t *self, bc_tag_humidity_state_t *state);
bool bc_tag_humidity_read_calibration(bc_tag_humidity_t *self);
bool bc_tag_humidity_power_up(bc_tag_humidity_t *self);
bool bc_tag_humidity_power_down(bc_tag_humidity_t *self);
bool bc_tag_humidity_one_shot_conversion(bc_tag_humidity_t *self);
bool bc_tag_humidity_read_result(bc_tag_humidity_t *self);
bool bc_tag_humidity_get_result(bc_tag_humidity_t *self, float *humidity);

#endif /* _BC_TAG_HUMIDITY_H */
