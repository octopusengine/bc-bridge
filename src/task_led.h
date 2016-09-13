#ifndef _TASK_LED_H
#define _TASK_LED_H

#include "bc_common.h"
#include "bc_os.h"
#include "bc_bridge.h"
#include "task.h"

// pri zmene zmenit i bc_talk bc_talk_led_state

typedef enum {
    TASK_LED_OFF,
    TASK_LED_ON,
    TASK_LED_1DOT,
    TASK_LED_2DOT,
    TASK_LED_3DOT

} task_led_state_t;

typedef struct
{
    bc_os_task_t task;
    bc_os_mutex_t mutex;
    bc_os_semaphore_t semaphore;

    bc_tick_t tick_feed_interval;
    bc_tick_t _tick_last_feed;
    bc_tick_t blink_interval;
    task_led_state_t state;

    bc_bridge_t *_bridge;
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

} task_led_t;




void task_led_spawn(bc_bridge_t *bridge, task_info_t *task_info);

void task_led_set_interval(task_led_t *self, bc_tick_t interval);
void task_led_get_interval(task_led_t *self, bc_tick_t *interval);
void task_led_set_state(task_led_t *self, task_led_state_t state);
void task_led_get_state(task_led_t *self, task_led_state_t *state);
void task_led_set_blink_interval(task_led_t *self, bc_tick_t interval);
void task_led_get_blink_interval(task_led_t *self, bc_tick_t *interval);

#endif /* _TASK_LED_H */
