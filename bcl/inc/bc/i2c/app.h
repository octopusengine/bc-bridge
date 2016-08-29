#ifndef _BC_I2C_APP_H
#define _BC_I2C_APP_H

#include <bc/tag.h>

#ifdef BRIDGE
#else
bc_tag_interface_t *bc_i2c_app_get_tag_interface(void);
#endif

#endif /* _BC_I2C_APP_H */
