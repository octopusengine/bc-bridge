#ifndef _BC_TAG_LUX_METER_H
#define _BC_TAG_LUX_METER_H

#include "bc_i2c.h"

#define BC_TAG_LUX_METER_ADDRESS_DEFAULT 0x44
#define BC_TAG_LUX_METER_ADDRESS_ALTERNATE 0x45

typedef struct
{
    bc_i2c_interface_t *_interface;
    uint8_t _device_address;
    bool _communication_fault;
    uint16_t _configuration;
    uint16_t _result;

} bc_tag_lux_meter_t;

typedef enum
{
    BC_TAG_LUX_METER_STATE_POWER_DOWN = 0,
    BC_TAG_LUX_METER_STATE_CONVERSION = 1,
    BC_TAG_LUX_METER_STATE_RESULT_READY = 2

} bc_tag_lux_meter_state_t;

bool bc_tag_lux_meter_init(bc_tag_lux_meter_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_tag_lux_meter_is_communication_fault(bc_tag_lux_meter_t *self);
bool bc_tag_lux_meter_get_state(bc_tag_lux_meter_t *self, bc_tag_lux_meter_state_t *state);
bool bc_tag_lux_meter_power_down(bc_tag_lux_meter_t *self);
bool bc_tag_lux_meter_single_shot_conversion(bc_tag_lux_meter_t *self);
bool bc_tag_lux_meter_continuous_conversion(bc_tag_lux_meter_t *self);
bool bc_tag_lux_meter_read_result(bc_tag_lux_meter_t *self);
bool bc_tag_lux_meter_get_result_lux(bc_tag_lux_meter_t *self, float *result_lux);

#endif /* _BC_TAG_LUX_METER_H */
