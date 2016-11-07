#ifndef _BC_TAG_BAROMETER_H
#define _BC_TAG_BAROMETER_H

#include "bc_i2c.h"

#define BC_TAG_BAROMETER_ADDRESS_DEFAULT 0x60

typedef enum
{
    BC_TAG_BAROMETER_STATE_POWER_DOWN = 0,
    BC_TAG_BAROMETER_STATE_CONVERSION = 1,
    BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE = 2,
    BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE = 3

} bc_tag_barometer_state_t;

typedef struct
{
    bc_i2c_interface_t *_interface;
    uint8_t _device_address;
    bool _communication_fault;
    bool disable_log;
    uint8_t _out_p_msb;
    uint8_t _out_p_csb;
    uint8_t _out_p_lsb;
    uint8_t _out_t_msb;
    uint8_t _out_t_lsb;

} bc_tag_barometer_t;

bool bc_tag_barometer_init(bc_tag_barometer_t *self, bc_i2c_interface_t *interface, uint8_t _device_address);
bool bc_tag_barometer_get_minimal_measurement_interval(bc_tag_barometer_t *self, bc_tick_t *interval);
bool bc_tag_barometer_is_communication_fault(bc_tag_barometer_t *self);
bool bc_tag_barometer_get_state(bc_tag_barometer_t *self, bc_tag_barometer_state_t *state);
bool bc_tag_barometer_power_down(bc_tag_barometer_t *self);
bool bc_tag_barometer_reset_and_power_down(bc_tag_barometer_t *self);
bool bc_tag_barometer_one_shot_conversion_altitude(bc_tag_barometer_t *self);
bool bc_tag_barometer_one_shot_conversion_pressure(bc_tag_barometer_t *self);
bool bc_tag_barometer_continuous_conversion_altitude(bc_tag_barometer_t *self);
bool bc_tag_barometer_continuous_conversion_pressure(bc_tag_barometer_t *self);

bool bc_tag_barometer_get_altitude(bc_tag_barometer_t *self, float *altitude_meter);
bool bc_tag_barometer_get_pressure(bc_tag_barometer_t *self, float *pressure_pascal);
bool bc_tag_barometer_get_temperature(bc_tag_barometer_t *self, float *temperature);

#endif /* _BC_TAG_BAROMETER_H */
