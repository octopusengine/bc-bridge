#ifndef _BC_TICK_H
#define _BC_TICK_H

#include <bc/common.h>

#define BC_TICK_MILLISECONDS(X) (X)
#define BC_TICK_SECONDS(X) ((X) * 1000UL)

typedef int32_t bc_tick_t;

bc_tick_t bc_tick_get(void);

#endif /* _BC_TICK_H */
