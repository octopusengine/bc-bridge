#include "bc_i2c.h"

void bc_i2c_transfer_init(bc_i2c_transfer_t *transfer)
{
    memset(transfer, 0, sizeof(*transfer));
}
