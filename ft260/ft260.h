#ifndef _FT260_H
#define _FT260_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    FT260_LED_STATE_OFF = 0,
    FT260_LED_STATE_ON = 1

} ft260_led_state_t;

typedef enum
{
    FT260_I2C_BUS_0 = 1,
    FT260_I2C_BUS_1 = 2

} ft260_i2c_bus_t;

bool ft260_open(void);
bool ft260_close(void);
bool ft260_check_chip_version(void);
bool ft260_led(ft260_led_state_t state);
bool ft260_i2c_reset(void);
bool ft260_i2c_get_bus_status(uint8_t *bus_status);
bool ft260_i2c_set_clock_speed(uint32_t speed);
bool ft260_i2c_get_clock_speed(uint32_t *speed);
bool ft260_i2c_write(uint8_t address, uint8_t *data, uint8_t length);
bool ft260_i2c_read(uint8_t address, uint8_t *data, uint8_t length);
bool ft260_i2c_set_bus(ft260_i2c_bus_t i2c_bus);
bool ft260_i2c_is_device_exists(uint8_t address);

void ft260_i2c_scan(void);

bool ft260_uart_reset(void);
bool ft260_uart_set_default_configuration(void);
void ft260_uart_print_configuration(void);
bool ft260_uart_write(uint8_t *data, uint8_t length);
uint8_t ft260_uart_read(uint8_t *data, uint8_t length);

// TODO Delete
void print_buffer(uint8_t *buffer, int length);

#endif
