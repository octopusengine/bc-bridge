#ifndef _TASK_RELAY_H
#define _TASK_RELAY_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "bc_module_relay.h"
#include "task.h"


typedef enum
{
    TASK_RELAY_STATE_FALSE = BC_MODULE_RELAY_STATE_T,
    TASK_RELAY_STATE_TRUE = BC_MODULE_RELAY_STATE_F,
    TASK_RELAY_STATE_NULL = -1

} task_relay_state_t;

typedef struct
{
    task_relay_state_t state;

} task_relay_parameters_t;

void *task_relay_worker(void *task_parameter);
void task_relay_set_state(task_info_t *task_info, task_relay_state_t state);
void task_relay_get_state(task_info_t *task_info, task_relay_state_t *state);

#endif /* _TASK_RELAY_H */
