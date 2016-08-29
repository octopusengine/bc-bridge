#ifndef BC_BRIDGE_SC16IS740_H
#define BC_BRIDGE_SC16IS740_H

#include <stdbool.h>
#include <stdint.h>
#include <bc/tag.h>

typedef struct
{
    bc_tag_interface_t *_interface;
    uint8_t _device_address;
    bool _communication_fault;

} bc_i2c_sc16is740_t;

bool bc_ic2_sc16is740_init(bc_i2c_sc16is740_t *self, bc_tag_interface_t *interface, uint8_t device_address);
bool bc_ic2_sc16is740_set_baudrate(bc_i2c_sc16is740_t *self, uint32_t baudrate);

#endif //BC_BRIDGE_SC16IS740_H
