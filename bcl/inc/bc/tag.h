#ifndef _BC_TAG_H
#define _BC_TAG_H

#include <bc/common.h>
#ifdef BRIDGE
#include <bc/bridge.h>
#endif


#ifdef BRIDGE

typedef bc_bridge_i2c_transfer_register_t bc_tag_transfer_t;

typedef struct
{
    bc_bridge_t *bridge;
    bc_bridge_i2c_channel_t channel;

} bc_tag_interface_t;

#else

typedef struct
{
    uint8_t device_address;
    uint16_t address;
    bool address_16_bit;
    uint8_t *buffer;
    uint16_t length;

} bc_tag_transfer_t;

typedef struct
{
    bool (*write)(bc_tag_transfer_t *transfer, bool *communication_fault);
    bool (*read)(bc_tag_transfer_t *transfer, bool *communication_fault);

} bc_tag_interface_t;
#endif

void bc_tag_transfer_init(bc_tag_transfer_t *transfer);

#endif
