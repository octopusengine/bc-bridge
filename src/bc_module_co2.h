#ifndef _BC_MODULE_CO2_H
#define _BC_MODULE_CO2_H

#include "bc_common.h"
#include "bc_tick.h"
#include "bc_i2c.h"
#include "bc_i2c_tca9534a.h"
#include "bc_i2c_sc16is740.h"

#define BC_MODULE_CO2_I2C_GPIO_EXPANDER_ADDRESS 0x38
#define BC_MODULE_CO2_I2C_UART_ADDRESS          0x4D

typedef enum
{
    BC_MODULE_CO2_STATE_ERROR = -1,
    BC_MODULE_CO2_STATE_INIT = 0,
    BC_MODULE_CO2_STATE_PRECHARGE = 1,
    BC_MODULE_CO2_STATE_IDLE = 2,
    BC_MODULE_CO2_STATE_READY = 3,
    BC_MODULE_CO2_STATE_CHARGE = 4,
    BC_MODULE_CO2_STATE_BOOT = 5,
    BC_MODULE_CO2_STATE_MEASURE = 6,
    BC_MODULE_CO2_STATE_SHUTDOWN = 7,
    BC_MODULE_CO2_STATE_CALIBRATION = 8

} bc_module_co2_state_t;

typedef enum
{
    BC_MODULE_CO2_CALIBRATION_ABC = 0x70,
    BC_MODULE_CO2_CALIBRATION_ABC_RF = 0x72,

} bc_module_co2_calibration_t;

typedef struct
{
    bool _first_measurement_done;
    bc_module_co2_state_t _state;
    bc_tick_t _t_state_timeout;
    bc_tick_t _t_calibration;
    uint8_t _sensor_state[23];
    bool _co2_concentration_unknown;
    int16_t _co2_concentration;
    uint8_t _rx_buffer[45];
    uint8_t _tx_buffer[33];
    bc_i2c_tca9534a_t _tca9534a;
    bc_i2c_sc16is740_t _sc16is740;
    uint16_t _pressure;
    bc_module_co2_calibration_t _calibration;
    bool _calibration_request;

} bc_module_co2_t;

bool bc_module_co2_init(bc_module_co2_t *self, bc_i2c_interface_t *interface);
void bc_module_co2_task(bc_module_co2_t *self);
bool bc_module_co2_task_get_concentration(bc_module_co2_t *self, int16_t *concentration);

bool bc_module_co2_charge_up(bc_module_co2_t *self);
bool bc_module_co2_charge_down(bc_module_co2_t *self);
bool bc_module_co2_power_down(bc_module_co2_t *self);
bool bc_module_co2_get_rdy(bc_module_co2_t *self, bc_i2c_tca9534a_value_t *value);
bool bc_module_co2_enable(bc_module_co2_t *self);
bool bc_module_co2_disable(bc_module_co2_t *self);
bool bc_module_co2_write_and_read(bc_module_co2_t *self, uint8_t length_write, uint8_t length_read);
bool bc_module_co2_calibration(bc_module_co2_t *self, bc_module_co2_calibration_t calibration);
void bc_module_co2_task_set_calibration_request(bc_module_co2_t *self, bc_module_co2_calibration_t calibration);
void bc_module_co2_task_set_pressure_kpa(bc_module_co2_t *self, float pressure);
bool bc_module_co2_task_is_state_error(bc_module_co2_t *self);
void bc_module_co2_task_get_feed_interval(bc_module_co2_t *self, bc_tick_t *feed_interval);


#endif /* _BC_MODULE_CO2_H */
