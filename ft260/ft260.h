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
bool ft260_i2c_set_clock_speed(uint32_t speed);
bool ft260_i2c_get_clock_speed(uint32_t *speed);
bool ft260_i2c_write(uint8_t address, uint8_t *data, uint8_t length);
bool ft260_i2c_read(uint8_t address, uint8_t *data, uint8_t length);
bool ft260_i2c_set_bus(ft260_i2c_bus_t i2c_bus);
bool ft260_i2c_is_device_exists(uint8_t address);

void ft260_i2c_scan(void);
// TODO Delete
void print_buffer(uint8_t *buffer, int length);

#if 0

bool ft260_open(void);
void ft260_close(void);
int ft260_check_chip_version();
void ft260_led(ft260_led_state_t led_state);

void ft260_i2c_reset();
bool ft260_i2c_set_clock_speed(uint32_t speed);
uint32_t ft260_i2c_get_clock_speed();
int ft260_i2c_write(uint8_t address, uint8_t *data, uint8_t length);
int ft260_i2c_read(uint8_t address, uint8_t *data, uint8_t length);
bool ft260_i2c_is_device_exist(uint8_t address);
void ft260_i2c_scan();
void ft260_i2c_set_bus(ft260_i2c_bus_t i2c_bus);

int ft260_uart_reset();
int ft260_uart_get_flow_ctrl();
int ft260_uart_get_baud_rate();
int ft260_uart_get_data_bit();
int ft260_uart_get_parity();
int ft260_uart_get_stop_bit();
int ft260_uart_get_breaking();
int ft260_uart_write(uint8_t *data, uint8_t length);
int ft260_uart_read(uint8_t *data, uint8_t length);

void print_buf(uint8_t* buf, int res);

#endif

#endif
