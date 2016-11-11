#ifndef BC_BRIDGE_TASK_MANAGER_H
#define BC_BRIDGE_TASK_MANAGER_H

#include "bc_bridge.h"
#include "task.h"

void task_manager_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length);
void task_manager_terminate(task_info_t *task_info_list, size_t length);
void task_manager_destroy_parameters(task_info_t *task_info_list, size_t length);

#endif //BC_BRIDGE_TASK_MANAGER_H
