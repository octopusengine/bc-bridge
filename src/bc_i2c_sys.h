#ifndef _BC_I2C_SYS_H
#define _BC_I2C_SYS_H

#include "bc_tag.h"

#ifdef BRIDGE
#else
bc_tag_interface_t *bc_i2c_sys_get_tag_interface(void);
#endif

#endif /* _BC_I2C_SYS_H */
