#ifndef _BC_MODULE_CO2_H
#define _BC_MODULE_CO2_H

#include "bc_common.h"
#include "bc_tick.h"
#include "bc_i2c.h"
#include "bc_i2c_tca9534a.h"
#include "bc_i2c_sc16is740.h"

#define BC_MODULE_CO2_MINIMAL_MEASUREMENT_INTERVAL_MS 16000L

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
	BC_MODULE_CO2_STATE_SHUTDOWN = 7

} bc_module_co2_state_t;

typedef struct
{
	bool _first_measurement_done;
	bc_module_co2_state_t _state;
	bc_tick_t _t_state_timeout;
	bc_tick_t _t_calibration;
	uint8_t _sensor_state[23];
	bool _co2_concentration_unknown;
	int16_t _co2_concentration;
	uint8_t _rx_buffer[64]; // TODO Trim size
	uint8_t _tx_buffer[64]; // TODO Trim size
	bc_i2c_tca9534a_t _tca9534a;
    bc_i2c_sc16is740_t _sc16is740;

} bc_module_co2_t;

bool bc_module_co2_init(bc_module_co2_t *self, bc_i2c_interface_t *interface);
void bc_module_co2_task(bc_module_co2_t *self);
bool bc_module_co2_get_concentration(bc_module_co2_t *self, int16_t *concentration);

#endif /* _BC_MODULE_CO2_H */
