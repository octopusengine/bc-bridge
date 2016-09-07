#ifndef _BC_I2C_H
#define _BC_I2C_H

#include "bc_common.h"
#ifdef BRIDGE
#include "bc_bridge.h"
#endif

#ifdef BRIDGE

typedef bc_bridge_i2c_transfer_t bc_i2c_transfer_t;

typedef struct
{
    bc_bridge_t *bridge;
    bc_bridge_i2c_channel_t channel;

} bc_i2c_interface_t;

#else

typedef struct
{
    uint8_t device_address;
    uint16_t address;
    bool address_16_bit;
    uint8_t *buffer;
    uint16_t length;

} bc_i2c_interface_t;

typedef struct
{
    bool (*write)(bc_i2c_transfer_t *transfer, bool *communication_fault);
    bool (*read)(bc_i2c_transfer_t *transfer, bool *communication_fault);

} bc_i2c_interface_t;

#endif

void bc_i2c_transfer_init(bc_i2c_transfer_t *transfer);

#endif
