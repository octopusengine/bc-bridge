#ifndef _BC_BRIDGE_H
#define _BC_BRIDGE_H

#include "bc_common.h"
#include "bc_os.h"

#define BC_BRIDGE_I2C_ONLY_READ 0xFF

typedef struct
{
    char *usb_path;
    char *path_i2c;
    char *path_uart;

} bc_bridge_device_info_t;

typedef enum
{
    BC_BRIDGE_I2C_CHANNEL_0 = 0,
    BC_BRIDGE_I2C_CHANNEL_1 = 1

} bc_bridge_i2c_channel_t;

typedef struct
{
    bc_bridge_device_info_t _info;
    bc_os_mutex_t _mutex;
    bc_bridge_i2c_channel_t _i2c_channel;
    int _fd_i2c;
    int _fd_uart;

} bc_bridge_t;

typedef struct
{
    bc_bridge_i2c_channel_t channel;
    uint8_t device_address;
    uint16_t address;
    bool address_16_bit;
    uint8_t *buffer;
    uint8_t length;
    bool disable_log;

} bc_bridge_i2c_transfer_t;

typedef enum
{
    BC_BRIDGE_LED_STATE_OFF = 0,
    BC_BRIDGE_LED_STATE_ON = 1

} bc_bridge_led_state_t;

bool bc_bridge_scan(bc_bridge_device_info_t *devices, uint8_t *device_count);
bool bc_bridge_open(bc_bridge_t *self, bc_bridge_device_info_t *info);
bool bc_bridge_close(bc_bridge_t *self);
bool bc_bridge_is_alive(bc_bridge_t *self);
bool bc_bridge_i2c_reset(bc_bridge_t *self);
bool bc_bridge_i2c_write(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_i2c_read(bc_bridge_t *self, bc_bridge_i2c_transfer_t *transfer);
bool bc_bridge_i2c_ping(bc_bridge_t *self, bc_bridge_i2c_channel_t channel, uint8_t device_address);
bool bc_bridge_led_set_state(bc_bridge_t *self, bc_bridge_led_state_t state);
bool bc_bridge_led_get_state(bc_bridge_t *self, bc_bridge_led_state_t *state);


#endif /* _BC_BRIDGE_H */
