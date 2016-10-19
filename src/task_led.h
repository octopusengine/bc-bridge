#ifndef _TASK_LED_H
#define _TASK_LED_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "task.h"

// pri zmene zmenit i bc_talk bc_talk_led_state

typedef enum
{
    TASK_LED_OFF,
    TASK_LED_ON,
    TASK_LED_1DOT,
    TASK_LED_2DOT,
    TASK_LED_3DOT

} task_led_state_t;

typedef struct
{
    bc_tick_t blink_interval;
    task_led_state_t state;

} task_led_parameters_t;

void *task_led_worker(void *task_parameter);
void task_led_set_state(task_info_t *task_info, task_led_state_t state);
void task_led_get_state(task_info_t *task_info, task_led_state_t *state);

#endif /* _TASK_LED_H */
