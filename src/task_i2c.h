#ifndef _TASK_I2C_H
#define _TASK_I2C_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_i2c.h"
#include "bc_bridge.h"
#include "task.h"
#include "bc_talk.h"

#define TASK_I2C_ACTIONS_LENGTH 10

typedef enum
{
    TASK_I2C_ACTION_TYPE_SCAN,
    TASK_I2C_ACTION_TYPE_READ,
    TASK_I2C_ACTION_TYPE_WRITE,

} task_i2c_action_type_t;

typedef struct
{
    task_i2c_action_type_t type;
    uint8_t channel;
    bc_talk_i2c_attributes_t *attributes;

} task_i2c_action_t;

typedef struct
{
    task_i2c_action_t actions[TASK_I2C_ACTIONS_LENGTH];
    uint8_t head;
    uint8_t tail;

} task_i2c_parameters_t;

void *task_i2c_worker(void *task_parameter);
bool task_i2c_set_scan(task_info_t *task_info, uint8_t channel);
bool task_i2c_set_command(task_info_t *task_info, task_i2c_action_type_t type, bc_talk_i2c_attributes_t *value);

#endif /* _TASK_I2C_H */
