#ifndef _TASK_THERMOMETER_H
#define _TASK_THERMOMETER_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"

void *task_thermometer_worker(void *task_parameter);

#endif /* _TASK_THERMOMETER_H */
