#include "bc_module_co2.h"
#include "bc_log.h"


#define BC_MODULE_CO2_BOOST_PIN              BC_I2C_TCA9534A_PIN_P1
#define BC_MODULE_CO2_VDD2_PIN               BC_I2C_TCA9534A_PIN_P2
#define BC_MODULE_CO2_EN_PIN                 BC_I2C_TCA9534A_PIN_P3
#define BC_MODULE_CO2_RDY_PIN                BC_I2C_TCA9534A_PIN_P7
#define BC_MODULE_CO2_UART_RESET_PIN         BC_I2C_TCA9534A_PIN_P6
#define BC_MODULE_CO2_TIMEOUT_PRECHARGE      60000
#define BC_MODULE_CO2_TIMEOUT_IDLE           10000
#define BC_MODULE_CO2_TIMEOUT_CHARGE         5000
#define BC_MODULE_CO2_TIMEOUT_BOOT           200
#define BC_MODULE_CO2_TIMEOUT_MEASURE        300
#define BC_MODULE_CO2_TIMEOUT_SHUTDOWN       200
#define BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS  0xFE
#define BC_MODULE_CO2_MODBUS_WRITE           0x41
#define BC_MODULE_CO2_MODBUS_READ            0x44
#define BC_MODULE_CO2_INITIAL_MEASUREMENT    0x10
#define BC_MODULE_CO2_SEQUENTIAL_MEASUREMENT 0x20
#define BC_MODULE_CO2_DEFAULT_PREASSURE      10124
#define BC_MODULE_CO2_RX_ERROR_STATUS0       (3+39)

static uint16_t _bc_module_co2_calculate_crc16(uint8_t *buffer, uint8_t length);

bool bc_module_co2_init(bc_module_co2_t *self, bc_i2c_interface_t *interface)
{
    memset(self, 0, sizeof(*self));

    self->_state = BC_MODULE_CO2_STATE_ERROR;
    self->_co2_concentration_unknown = true;

    bc_log_debug("bc_module_co2_init: initialization start");

    if (!bc_i2c_tca9534a_init(&self->_tca9534a, interface, BC_MODULE_CO2_I2C_GPIO_EXPANDER_ADDRESS))
    {
        bc_log_debug("bc_module_co2_init: call failed: bc_ic2_tca9534a_init");

        return false;
    }

    bc_log_debug("bc_module_co2_init: bc_ic2_tca9534a_set_modes");

    if (!bc_i2c_tca9534a_set_port_direction(&self->_tca9534a, BC_I2C_TCA9534A_DIRECTION_ALL_INPUT))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_ic2_tca9534a_set_modes");

        return false;
    }

    if (!bc_i2c_tca9534a_write_port(&self->_tca9534a, BC_I2C_TCA9534A_VALUE_ALL_LOW))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_ic2_tca9534a_write_pins");

        return false;
    }

    //reset uart
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_UART_RESET_PIN,
                                           BC_I2C_TCA9534A_DIRECTION_OUTPUT))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_i2c_tca9534a_set_pin_direction UART_RESET OUTPUT");

        return false;
    }
    bc_os_task_sleep(1);

    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_UART_RESET_PIN,
                                           BC_I2C_TCA9534A_DIRECTION_INPUT))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_i2c_tca9534a_set_pin_direction UART_RESET INPUT");

        return false;
    }
    bc_os_task_sleep(1);

    if (!bc_ic2_sc16is740_init(&self->_sc16is740, interface, BC_MODULE_CO2_I2C_UART_ADDRESS))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_ic2_sc16is740_init");

        return false;
    }

    if (!bc_ic2_sc16is740_init(&self->_sc16is740, interface, BC_MODULE_CO2_I2C_UART_ADDRESS))
    {
        bc_log_error("bc_module_co2_init: call failed: bc_ic2_sc16is740_init");
        return false;
    }

    self->_state = BC_MODULE_CO2_STATE_INIT;

    self->_pressure = BC_MODULE_CO2_DEFAULT_PREASSURE;

    bc_log_debug("bc_module_co2_init: initialization end");

    return true;
}

void bc_module_co2_task(bc_module_co2_t *self)
{
    bc_log_debug("bc_module_co2_task: state %d ", self->_state);

    bc_tick_t t_now;
    bc_i2c_tca9534a_value_t rdy_pin_value;

    t_now = bc_tick_get();

    switch (self->_state)
    {
        case BC_MODULE_CO2_STATE_INIT:
        {

            if (bc_module_co2_charge_up(self))
            {
                self->_state = BC_MODULE_CO2_STATE_PRECHARGE;

                self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_PRECHARGE;
            }
            else
            {
                self->_state = BC_MODULE_CO2_STATE_ERROR;
            }

            break;
        }
        case BC_MODULE_CO2_STATE_PRECHARGE:
        {
            bc_log_debug("bc_module_co2_task: precharge timeout %d ", self->_t_state_timeout - t_now);

            if ((t_now - self->_t_state_timeout) >= 0)
            {
                if (bc_module_co2_charge_down(self))
                {

                    self->_state = BC_MODULE_CO2_STATE_IDLE;

                    self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_IDLE;
                }
                else
                {
                    self->_state = BC_MODULE_CO2_STATE_ERROR;
                }
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

            if (bc_module_co2_charge_up(self))
            {
                self->_state = BC_MODULE_CO2_STATE_CHARGE;

                self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_CHARGE;
            }
            else
            {
                self->_state = BC_MODULE_CO2_STATE_ERROR;
            }

            break;
        }
        case BC_MODULE_CO2_STATE_CHARGE:
        {
            if ((t_now - self->_t_state_timeout) >= 0)
            {

                if (bc_module_co2_enable(self))
                {
                    self->_state = BC_MODULE_CO2_STATE_BOOT;

                    self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_BOOT;
                }
                else
                {
                    self->_state = BC_MODULE_CO2_STATE_ERROR;
                }
            }

            break;
        }
        case BC_MODULE_CO2_STATE_BOOT:
        {

            if (!bc_module_co2_get_rdy(self, &rdy_pin_value))
            {
                bc_log_error("bc_module_co2_task: STATE_BOOT: call failed: bc_module_co2_get_rdy");
                self->_state = BC_MODULE_CO2_STATE_ERROR;
                break;
            }

            if (rdy_pin_value == BC_I2C_TCA9534A_VALUE_LOW)
            {

                if (self->_calibration_request)
                {
                    bc_module_co2_calibration(self, self->_calibration);

                    self->_calibration_request = false;

                    self->_state = BC_MODULE_CO2_STATE_CALIBRATION;

                    break;
                }

                memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));
                uint8_t length;

                if (!self->_first_measurement_done)
                {
                    self->_tx_buffer[0] = BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS;
                    self->_tx_buffer[1] = BC_MODULE_CO2_MODBUS_WRITE;
                    self->_tx_buffer[2] = 0x00;
                    self->_tx_buffer[3] = 0x80;
                    self->_tx_buffer[4] = 0x01;
                    self->_tx_buffer[5] = BC_MODULE_CO2_INITIAL_MEASUREMENT;
                    self->_tx_buffer[6] = 0x28;//crc low
                    self->_tx_buffer[7] = 0x7E;//crc high

                    length = 8;
                }
                else
                {
                    uint16_t crc16;

                    self->_tx_buffer[0] = BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS;
                    self->_tx_buffer[1] = BC_MODULE_CO2_MODBUS_WRITE;
                    self->_tx_buffer[2] = 0x00;
                    self->_tx_buffer[3] = 0x80;
                    self->_tx_buffer[4] = 0x1A;//26
                    self->_tx_buffer[5] = BC_MODULE_CO2_SEQUENTIAL_MEASUREMENT;

                    //copy previous measurement data
                    memcpy(&self->_tx_buffer[6], self->_sensor_state, 23);

                    self->_tx_buffer[29] = (uint8_t) (self->_pressure >> 8);
                    self->_tx_buffer[30] = (uint8_t) self->_pressure;

                    crc16 = _bc_module_co2_calculate_crc16(self->_tx_buffer, 31);

                    self->_tx_buffer[31] = (uint8_t) crc16;
                    self->_tx_buffer[32] = (uint8_t) (crc16 >> 8);

                    length = 33;
                }

                if (!bc_module_co2_write_and_read(self, length, 4))
                {
                    bc_log_error("bc_module_co2_task: STATE_BOOT, call failed: bc_module_co2_write_and_read");
                    self->_state = BC_MODULE_CO2_STATE_ERROR;
                    break;
                }

                self->_state = BC_MODULE_CO2_STATE_MEASURE;

                self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_MEASURE;
            }
            else if ((t_now - self->_t_state_timeout) >= 0)
            {
                self->_state = BC_MODULE_CO2_STATE_ERROR;
            }

            break;
        }
        case BC_MODULE_CO2_STATE_MEASURE:
        {

            if (!bc_module_co2_get_rdy(self, &rdy_pin_value))
            {
                bc_log_error("bc_module_co2_task: STATE_MEASURE: call failed: bc_module_co2_get_rdy");
                self->_state = BC_MODULE_CO2_STATE_ERROR;
                break;
            }

            if (rdy_pin_value == BC_I2C_TCA9534A_VALUE_HIGH)
            {
                memset(self->_tx_buffer, 0, sizeof(self->_tx_buffer));

                self->_tx_buffer[0] = BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS;
                self->_tx_buffer[1] = BC_MODULE_CO2_MODBUS_READ;
                self->_tx_buffer[2] = 0x00;
                self->_tx_buffer[3] = 0x80;
                self->_tx_buffer[4] = 0x28;//40
                self->_tx_buffer[5] = 0x78;
                self->_tx_buffer[6] = 0xFA;

                if (!bc_module_co2_write_and_read(self, 7, (40 + 5)))
                {
                    bc_log_error("bc_module_co2_task: STATE_MEASURE, call failed: bc_module_co2_write_and_read");
                    self->_state = BC_MODULE_CO2_STATE_ERROR;
                    break;
                }

                if (self->_rx_buffer[BC_MODULE_CO2_RX_ERROR_STATUS0] != 0)
                {
                    self->_co2_concentration_unknown = true;

                    self->_state = BC_MODULE_CO2_STATE_ERROR;
                }
                else
                {
                    memcpy(self->_sensor_state, &self->_rx_buffer[4], 23);

                    self->_first_measurement_done = true;

                    self->_co2_concentration = ((int16_t) self->_rx_buffer[29]) << 8;
                    self->_co2_concentration |= (int16_t) self->_rx_buffer[30];

                    self->_co2_concentration_unknown = false;

                    self->_state = BC_MODULE_CO2_STATE_SHUTDOWN;

                    self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_SHUTDOWN;
                }

            }
            else if ((t_now - self->_t_state_timeout) >= 0)
            {
                self->_state = BC_MODULE_CO2_STATE_ERROR;
            }

            break;
        }
        case BC_MODULE_CO2_STATE_SHUTDOWN:
        {

            if (bc_module_co2_power_down(self))
            {
                self->_state = BC_MODULE_CO2_STATE_IDLE;

                self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_IDLE;
            }
            else
            {
                self->_state = BC_MODULE_CO2_STATE_ERROR;
            }

            break;
        }
        case BC_MODULE_CO2_STATE_CALIBRATION:
        {
            if (!bc_module_co2_get_rdy(self, &rdy_pin_value))
            {
                bc_log_error("bc_module_co2_task: STATE_MEASURE: call failed: bc_module_co2_get_rdy");
                self->_state = BC_MODULE_CO2_STATE_ERROR;
                break;
            }

            if (rdy_pin_value == BC_I2C_TCA9534A_VALUE_HIGH)
            {
                self->_state = BC_MODULE_CO2_STATE_SHUTDOWN;

                self->_t_state_timeout = t_now + BC_MODULE_CO2_TIMEOUT_SHUTDOWN;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    if (self->_state == BC_MODULE_CO2_STATE_ERROR)
    {
        self->_first_measurement_done = false;
        self->_co2_concentration_unknown = true;
    }
}

bool bc_module_co2_task_get_concentration(bc_module_co2_t *self, int16_t *concentration)
{
    if (self->_co2_concentration_unknown)
    {
        return false;
    }

    *concentration = self->_co2_concentration;

    return true;
}

bool bc_module_co2_charge_up(bc_module_co2_t *self)
{
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_VDD2_PIN, BC_I2C_TCA9534A_DIRECTION_OUTPUT))
    {
        bc_log_error("bc_module_co2_charge_up: call failed: bc_i2c_tca9534a_set_pin_direction VDD2_PIN OUTPUT");

        return false;
    }

    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_BOOST_PIN, BC_I2C_TCA9534A_DIRECTION_OUTPUT))
    {
        bc_log_error("bc_module_co2_charge_up: call failed: bc_i2c_tca9534a_set_pin_direction BOOST_PIN OUTPUT");

        return false;
    }

    return true;
}

bool bc_module_co2_charge_down(bc_module_co2_t *self)
{
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_BOOST_PIN, BC_I2C_TCA9534A_DIRECTION_INPUT))
    {
        bc_log_error("bc_module_co2_charge_down: call failed: bc_i2c_tca9534a_set_pin_direction: BOOST_PIN INPUT");

        return false;
    }
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_VDD2_PIN, BC_I2C_TCA9534A_DIRECTION_INPUT))
    {
        bc_log_error("bc_module_co2_charge_down: call failed: bc_i2c_tca9534a_set_pin_direction: VDD2_PIN INPUT");

        return false;
    }
    return true;
}

bool bc_module_co2_power_down(bc_module_co2_t *self)
{
    if (!bc_module_co2_disable(self))
    {
        return false;
    }

    bc_os_task_sleep(1);

    return bc_module_co2_charge_down(self);
}

bool bc_module_co2_get_rdy(bc_module_co2_t *self, bc_i2c_tca9534a_value_t *value)
{
    if (!bc_i2c_tca9534a_read_pin(&self->_tca9534a, BC_MODULE_CO2_RDY_PIN, value))
    {
        bc_log_error("bc_module_co2_get_rdy: call failed: bc_i2c_tca9534a_read_pin RDY_PIN");
        return false;
    }

    return true;
}

bool bc_module_co2_enable(bc_module_co2_t *self)
{
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_EN_PIN, BC_I2C_TCA9534A_DIRECTION_OUTPUT))
    {
        bc_log_error("bc_module_co2_enable: call failed: bc_i2c_tca9534a_set_pin_direction EN_PIN OUTPUT");

        return false;
    }

    return true;
}

bool bc_module_co2_disable(bc_module_co2_t *self)
{
    if (!bc_i2c_tca9534a_set_pin_direction(&self->_tca9534a, BC_MODULE_CO2_EN_PIN, BC_I2C_TCA9534A_DIRECTION_INPUT))
    {
        bc_log_error("bc_module_co2_disable: call failed: bc_i2c_tca9534a_set_pin_direction EN_PIN INPUT");

        return false;
    }

    return true;
}

bool bc_module_co2_write_and_read(bc_module_co2_t *self, uint8_t length_write, uint8_t length_read)
{
    if (!bc_ic2_sc16is740_reset_fifo(&self->_sc16is740, BC_I2C_SC16IS740_FIFO_RX))
    {
        bc_log_error("bc_module_co2_write_and_read: call failed: bc_ic2_sc16is740_reset_fifo RX");
        return false;
    }

    if (!bc_ic2_sc16is740_write(&self->_sc16is740, self->_tx_buffer, length_write))
    {
        bc_log_error("bc_module_co2_write_and_read, call failed: bc_ic2_sc16is740_write");
        return false;
    }

    if (length_read > 0)
    {
        if (!bc_ic2_sc16is740_read(&self->_sc16is740, self->_rx_buffer, length_read, 100))
        {
            bc_log_error("bc_module_co2_write_and_read, call failed: bc_ic2_sc16is740_read");
            return false;
        }

        if (self->_rx_buffer[0] != BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS)
        {
            bc_log_error("bc_module_co2_write_and_read, response bad device adress");
            return false;
        }

        if (self->_rx_buffer[1] != self->_tx_buffer[1])
        {
            bc_log_error("bc_module_co2_write_and_read, different function code request and response");
            return false;
        }

        if (_bc_module_co2_calculate_crc16(self->_rx_buffer, length_read) != 0)
        {
            bc_log_error("bc_module_co2_write_and_read, response bad crc ");
            return false;
        }
    }

    return true;
}

bool bc_module_co2_calibration(bc_module_co2_t *self, bc_module_co2_calibration_t calibration)
{
    uint16_t crc16;

    self->_tx_buffer[0] = BC_MODULE_CO2_MODBUS_DEVICE_ADDRESS;
    self->_tx_buffer[1] = BC_MODULE_CO2_MODBUS_WRITE;
    self->_tx_buffer[2] = 0x00;
    self->_tx_buffer[3] = 0x80;
    self->_tx_buffer[4] = 0x01;
    self->_tx_buffer[5] = calibration;

    crc16 = _bc_module_co2_calculate_crc16(self->_tx_buffer, 6);

    self->_tx_buffer[6] = (uint8_t) crc16;
    self->_tx_buffer[7] = (uint8_t) (crc16 >> 8);


    if (!bc_module_co2_write_and_read(self, 8, 4))
    {
        bc_log_error("bc_module_co2_calibration, call failed: bc_module_co2_write_and_read");
        self->_state = BC_MODULE_CO2_STATE_ERROR;
        return false;
    }

    return true;
}

void bc_module_co2_task_set_pressure_kpa(bc_module_co2_t *self, float pressure)
{
    self->_pressure = (uint16_t) (pressure * 100);
}

bool bc_module_co2_task_is_state_error(bc_module_co2_t *self)
{
    return self->_state == BC_MODULE_CO2_STATE_ERROR;
}

void bc_module_co2_task_get_feed_interval(bc_module_co2_t *self, bc_tick_t *feed_interval)
{
    *feed_interval = self->_t_state_timeout - bc_tick_get();
}

void bc_module_co2_task_set_calibration_request(bc_module_co2_t *self, bc_module_co2_calibration_t calibration)
{
    self->_calibration_request = true;
    self->_calibration = calibration;
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
