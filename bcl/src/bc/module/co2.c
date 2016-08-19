#include <bc/module/co2.h>
#include <bc/i2c/sys.h>
#include <ft260.h>


// TODO Implement ABC calibration
// TODO Extract all constants to the macros in the beginning
// TODO Implement possibility to add own interface

#define BOOST_Pin BC_I2C_TCA9534A_PIN5
#define EN_Pin BC_I2C_TCA9534A_PIN4
#define RDY_Pin BC_I2C_TCA9534A_PIN0

static uint16_t _bc_module_co2_calculate_crc16(uint8_t *buffer, uint8_t length);

void bc_module_co2_init(bc_module_co2_t *self)
{
	memset(self, 0, sizeof(*self));

	self->_state = BC_MODULE_CO2_STATE_INIT;
	self->_co2_concentration_unknown = true;
	br_ic2_tca9534a_init(self->_tca9534a, bc_i2c_sys_get_tag_interface(), 0x38);
	
	bc_ic2_tca9534a_set_mode(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_OUTPUT);
	bc_ic2_tca9534a_set_mode(self->_tca9534a, EN_Pin, BC_I2C_TCA9534A_OUTPUT);
	bc_ic2_tca9534a_set_mode(self->_tca9534a, RDY_Pin, BC_I2C_TCA9534A_INPUT);
}

void bc_module_co2_task(bc_module_co2_t *self)
{
	bc_tick_t t_now;
	bc_i2c_tca9534a_value_t rdy_pin_value;

	t_now = bc_tick_get();

	switch (self->_state)
	{
		case BC_MODULE_CO2_STATE_INIT:
		{
			// TODO Adjust time to > 1min
			self->_state = BC_MODULE_CO2_STATE_PRECHARGE;
			self->_t_state_timeout = t_now + BC_TICK_SECONDS(180);

			bc_ic2_tca9534a_write_pin(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_HIGH);

			break;
		}
		case BC_MODULE_CO2_STATE_PRECHARGE:
		{
			if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_IDLE;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);

				bc_ic2_tca9534a_write_pin(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_LOW);
			}

			break;
		}
		case BC_MODULE_CO2_STATE_IDLE:
		{
			if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_READY;
			}

			break;
		}
		case BC_MODULE_CO2_STATE_READY:
		{
			self->_state = BC_MODULE_CO2_STATE_CHARGE;
			self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);

			bc_ic2_tca9534a_write_pin(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_HIGH);

			break;
		}
		case BC_MODULE_CO2_STATE_CHARGE:
		{
			if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_BOOT;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);

				bc_ic2_tca9534a_write_pin(self->_tca9534a, EN_Pin, BC_I2C_TCA9534A_HIGH);
			}

			break;
		}
		case BC_MODULE_CO2_STATE_BOOT:
		{

			if (  bc_ic2_tca9534a_read_pin(self->_tca9534a, RDY_Pin, &rdy_pin_value) && 
			(rdy_pin_value == BC_I2C_TCA9534A_LOW ))
			{
				if (!self->_first_measurement_done)
				{
					memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));

					self->_tx_buffer[0] = 0xFE;
					self->_tx_buffer[1] = 0x41;
					self->_tx_buffer[2] = 0x00;
					self->_tx_buffer[3] = 0x80;
					self->_tx_buffer[4] = 0x01;
					self->_tx_buffer[5] = 0x10;
					self->_tx_buffer[6] = 0x28;
					self->_tx_buffer[7] = 0x7E;

					if (ft260_uart_write(self->_tx_buffer, 8))
					{
						self->_state = BC_MODULE_CO2_STATE_ERROR;
						break;
					}
				}
				else
				{
					uint16_t crc16;

					memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));

					self->_tx_buffer[0] = 0xFE;
					self->_tx_buffer[1] = 0x41;
					self->_tx_buffer[2] = 0x00;
					self->_tx_buffer[3] = 0x80;
					self->_tx_buffer[4] = 0x1A;
					self->_tx_buffer[5] = 0x20;

					memcpy(&self->_tx_buffer[6], self->_sensor_state, 23);

					self->_tx_buffer[29] = 0x27;
					self->_tx_buffer[30] = 0x8D;

					crc16 = _bc_module_co2_calculate_crc16(self->_tx_buffer, 31);

					self->_tx_buffer[31] = (uint8_t) crc16;
					self->_tx_buffer[32] = (uint8_t) (crc16 >> 8);

					if (!ft260_uart_write(self->_tx_buffer, 33))
					{
						self->_state = BC_MODULE_CO2_STATE_ERROR;
						break;
					}
				}

				if (ft260_uart_read(self->_rx_buffer, 4) != 4)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				if (self->_rx_buffer[0] != 0xFE || self->_rx_buffer[1] != 0x41)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				if (_bc_module_co2_calculate_crc16(self->_rx_buffer, 4) != 0)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				self->_state = BC_MODULE_CO2_STATE_MEASURE;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(10);
			}
			else if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_ERROR;
			}

			break;
		}
		case BC_MODULE_CO2_STATE_MEASURE:
		{

			if ( bc_ic2_tca9534a_read_pin(self->_tca9534a, RDY_Pin, &rdy_pin_value) && 
			(rdy_pin_value == BC_I2C_TCA9534A_HIGH) )
			{
				memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));

				self->_tx_buffer[0] = 0xFE;
				self->_tx_buffer[1] = 0x44;
				self->_tx_buffer[2] = 0x00;
				self->_tx_buffer[3] = 0x80;
				self->_tx_buffer[4] = 0x28;
				self->_tx_buffer[5] = 0x78;
				self->_tx_buffer[6] = 0xFA;

				if (!ft260_uart_write(self->_tx_buffer, 7))
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				if (ft260_uart_read(self->_rx_buffer, 45) != 45)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				if (_bc_module_co2_calculate_crc16(self->_rx_buffer, 45) != 0)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				memcpy(self->_sensor_state, &self->_rx_buffer[4], 23);

				self->_first_measurement_done = true;

				self->_co2_concentration = ((int16_t) self->_rx_buffer[29]) << 8;
				self->_co2_concentration |= (int16_t) self->_rx_buffer[30];

				// TODO Is this bit logical OR of all the errors?
				if ((self->_rx_buffer[42] & 0x01) == 0)
				{
					self->_co2_concentration_unknown = false;
				}
				else
				{
					self->_co2_concentration_unknown = true;
				}

				self->_state = BC_MODULE_CO2_STATE_SHUTDOWN;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);
			}
			else if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_ERROR;
			}

			break;
		}
		case BC_MODULE_CO2_STATE_SHUTDOWN:
		{
			self->_state = BC_MODULE_CO2_STATE_IDLE;
			self->_t_state_timeout = t_now + BC_TICK_SECONDS(30);

			// TODO Split these two operations
			bc_ic2_tca9534a_write_pin(self->_tca9534a, EN_Pin, BC_I2C_TCA9534A_LOW);
			bc_ic2_tca9534a_write_pin(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_LOW);

			break;
		}
		case BC_MODULE_CO2_STATE_ERROR:
		{
			bc_ic2_tca9534a_write_pin(self->_tca9534a, EN_Pin, BC_I2C_TCA9534A_LOW);
			bc_ic2_tca9534a_write_pin(self->_tca9534a, BOOST_Pin, BC_I2C_TCA9534A_LOW);

			self->_state = BC_MODULE_CO2_STATE_PRECHARGE;
			self->_t_state_timeout = t_now + BC_TICK_SECONDS(30);
			self->_first_measurement_done = false;
			self->_co2_concentration_unknown = true;

			break;
		}
		default:
		{
			break;
		}
	}
}

bool bc_module_co2_get_concentration(bc_module_co2_t *self, int16_t *concentration)
{
	if (self->_co2_concentration_unknown)
	{
		return false;
	}

	*concentration = self->_co2_concentration;

	return true;
}

static uint16_t _bc_module_co2_calculate_crc16(uint8_t *buffer, uint8_t length)
{
	uint16_t crc16;

	for (crc16 = 0xFFFF; length != 0; length--, buffer++)
	{
		uint8_t i;

		crc16 ^= *buffer;

		for (i = 0; i < 8; i++)
		{
			if ((crc16 & 0x0001) != 0)
			{
				crc16 >>= 1;
				crc16 ^= 0xA001;
			}
			else
			{
				crc16 >>= 1;
			}
		}
	}

	return crc16;
}
