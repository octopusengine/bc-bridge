#ifndef _BC_TAG_HUMIDITY_H
#define _BC_TAG_HUMIDITY_H

#include "bc_i2c.h"

#define BC_TAG_HUMIDITY_DEVICE_ADDRESS_DEFAULT 0x5F

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
    uint8_t _device_address;
    bool _communication_fault;
    bool _calibration_not_read;

    int16_t h0_rh;
    int16_t h0_t0_out;
    float h_grad;


} bc_tag_humidity_t;

bool bc_tag_humidity_init(bc_tag_humidity_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_tag_humidity_is_communication_fault(bc_tag_humidity_t *self);
bool bc_tag_humidity_get_state(bc_tag_humidity_t *self, bc_tag_humidity_state_t *state);
bool bc_tag_humidity_load_calibration(bc_tag_humidity_t *self);
bool bc_tag_humidity_power_up(bc_tag_humidity_t *self);
bool bc_tag_humidity_power_down(bc_tag_humidity_t *self);
bool bc_tag_humidity_one_shot_conversion(bc_tag_humidity_t *self);
bool bc_tag_humidity_continuous_conversion(bc_tag_humidity_t *self);
bool bc_tag_humidity_get_relative_humidity(bc_tag_humidity_t *self, float *humidity);

#endif /* _BC_TAG_HUMIDITY_H */
