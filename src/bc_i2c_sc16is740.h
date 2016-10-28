/**
 * Single UART with I2C-bus interface, 64 bytes of transmit and receive FIFOs
 */

#ifndef _BC_I2C_SC16IS740_H
#define _BC_I2C_SC16IS740_H

#include "bc_common.h"
#include "bc_i2c.h"

typedef enum
{
    BC_I2C_SC16IS740_FIFO_RX = 0x02,
    BC_I2C_SC16IS740_FIFO_TX = 0x04

} bc_i2c_sc16is740_fifo_t;

typedef struct
{
    bc_i2c_interface_t *_interface;
    uint8_t _device_address;
    bool _communication_fault;

} bc_i2c_sc16is740_t;

bool bc_ic2_sc16is740_init(bc_i2c_sc16is740_t *self, bc_i2c_interface_t *interface, uint8_t device_address);
bool bc_ic2_sc16is740_reset_device(bc_i2c_sc16is740_t *self);
bool bc_ic2_sc16is740_reset_fifo(bc_i2c_sc16is740_t *self, bc_i2c_sc16is740_fifo_t fifo);
bool bc_ic2_sc16is740_read(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length, bc_tick_t timeout_milis);
bool bc_ic2_sc16is740_write(bc_i2c_sc16is740_t *self, uint8_t *data, uint8_t length);

#endif /* _BC_I2C_SC16IS740_H */
