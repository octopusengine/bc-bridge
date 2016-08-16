#include <bc/module/co2.h>
#include <stm32l0xx_hal.h>

// TODO Implement ABC calibration
// TODO Extract all constants to the macros in the beginning
// TODO Implement possibility to add own interface

extern UART_HandleTypeDef huart1;

static uint16_t _bc_module_co2_calculate_crc16(uint8_t *buffer, uint8_t length);

void bc_module_co2_init(bc_module_co2_t *self)
{
	memset(self, 0, sizeof(*self));

	self->_state = BC_MODULE_CO2_STATE_INIT;
	self->_co2_concentration_unknown = true;
}

void bc_module_co2_task(bc_module_co2_t *self)
{
	bc_tick_t t_now;

	t_now = bc_tick_get();

	switch (self->_state)
	{
		case BC_MODULE_CO2_STATE_INIT:
		{
			// TODO Adjust time to > 1min
			self->_state = BC_MODULE_CO2_STATE_PRECHARGE;
			self->_t_state_timeout = t_now + BC_TICK_SECONDS(180);

			HAL_GPIO_WritePin(BOOST_GPIO_Port, BOOST_Pin, GPIO_PIN_SET);

			break;
		}
		case BC_MODULE_CO2_STATE_PRECHARGE:
		{
			if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_IDLE;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);

				HAL_GPIO_WritePin(BOOST_GPIO_Port, BOOST_Pin, GPIO_PIN_RESET);
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

			HAL_GPIO_WritePin(BOOST_GPIO_Port, BOOST_Pin, GPIO_PIN_SET);

			break;
		}
		case BC_MODULE_CO2_STATE_CHARGE:
		{
			if ((t_now - self->_t_state_timeout) >= 0)
			{
				self->_state = BC_MODULE_CO2_STATE_BOOT;
				self->_t_state_timeout = t_now + BC_TICK_SECONDS(5);

				HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);
			}

			break;
		}
		case BC_MODULE_CO2_STATE_BOOT:
		{
			if (HAL_GPIO_ReadPin(RDY_GPIO_Port, RDY_Pin) == GPIO_PIN_RESET)
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

					if (HAL_UART_Transmit(&huart1, self->_tx_buffer, 8, 100) != HAL_OK)
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

					if (HAL_UART_Transmit(&huart1, self->_tx_buffer, 33, 100) != HAL_OK)
					{
						self->_state = BC_MODULE_CO2_STATE_ERROR;
						break;
					}
				}

				if (HAL_UART_Receive(&huart1, self->_rx_buffer, 4, 100) != HAL_OK)
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
			if (HAL_GPIO_ReadPin(RDY_GPIO_Port, RDY_Pin) == GPIO_PIN_SET)
			{
				memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));

				self->_tx_buffer[0] = 0xFE;
				self->_tx_buffer[1] = 0x44;
				self->_tx_buffer[2] = 0x00;
				self->_tx_buffer[3] = 0x80;
				self->_tx_buffer[4] = 0x28;
				self->_tx_buffer[5] = 0x78;
				self->_tx_buffer[6] = 0xFA;

				if (HAL_UART_Transmit(&huart1, self->_tx_buffer, 7, 100) != HAL_OK)
				{
					self->_state = BC_MODULE_CO2_STATE_ERROR;
					break;
				}

				if (HAL_UART_Receive(&huart1, self->_rx_buffer, 45, 100) != HAL_OK)
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
			HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BOOST_GPIO_Port, BOOST_Pin, GPIO_PIN_RESET);

			break;
		}
		case BC_MODULE_CO2_STATE_ERROR:
		{
			HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(BOOST_GPIO_Port, BOOST_Pin, GPIO_PIN_SET);

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
