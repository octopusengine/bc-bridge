#ifndef _BC_BRIDGE_H
#define _BC_BRIDGE_H

#include <bc/common.h>
#include <bc/os.h>

typedef struct
{
    char *usb_path;
    char *i2c_hid_path;
    char *uart_hid_path;

} bc_bridge_device_info_t;

typedef enum
{
    BC_BRIDGE_I2C_CHANNEL_0 = 0,
    BC_BRIDGE_I2C_CHANNEL_1 = 1

} bc_bridge_i2c_channel_t;

typedef struct
{
    bc_bridge_device_info_t _info;
    bc_os_mutex_t _i2c_mutex;
    bc_bridge_i2c_channel_t _i2c_channel;
    int _i2c_fd_hid;
    int _uart_fd_hid;


} bc_bridge_t;

typedef struct
{
    bc_bridge_i2c_channel_t channel;
    uint8_t device_address;
    uint8_t *buffer;
    uint8_t length;

} bc_bridge_i2c_transfer_t;

typedef enum
{
    BC_BRIDGE_LED_STATE_OFF = 0,
    BC_BRIDGE_LED_STATE_ON = 1

} bc_bridge_led_state_t;

bool bc_bridge_scan(bc_bridge_device_info_t *devices, uint8_t *length);
bool bc_bridge_open(bc_bridge_t *self, bc_bridge_device_info_t *info);
bool bc_bridge_close(bc_bridge_t *self);
void bc_bridge_i2c_lock(bc_bridge_t *self);
void bc_bridge_i2c_unlock(bc_bridge_t *self);
bool bc_bridge_i2c_write(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_i2c_read(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_led_set_state(bc_bridge_t *self, bc_bridge_led_state_t state);

#endif
