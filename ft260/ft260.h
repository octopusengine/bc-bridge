#include <stdint.h>

#define VENDOR_ID 0x0403
#define DEVICE_ID 0x6030

#define REPORT_ID_CHIP_CODE 0xA0
#define REPORT_ID_SYSTEM_SETTING 0xA1
#define REPORT_ID_GPIO 0xB0
#define REPORT_ID_I2C_STATUS 0xC0
#define REPORT_ID_UART_STATUS 0xE0

enum I2C_BUS { SYS=1, APP=2 };

int ft260_open_device();
void ft260_close_dev();
int ft260_check_chip_version();
void ft260_led(int state);

void ft260_i2c_reset();
int ft260_i2c_set_clock_speed(int speed);
int ft260_i2c_get_clock_speed();
int ft260_i2c_write(unsigned char address, char *data, char data_length);
int ft260_i2c_read(unsigned char address, char *data, char data_length);
int ft260_i2c_check_device_exist(unsigned char address)
void ft260_i2c_scan();
void ft260_i2c_set_bus(enum I2C_BUS bus);

void print_buf(char* buf, int res);