#ifndef BC_BRIDGE_TASK_H
#define BC_BRIDGE_TASK_H

#include "bc_bridge.h"

typedef enum
{
    TASK_CLASS_SENSOR,
    TASK_CLASS_ACTUATOR,
    TASK_CLASS_BRIDGE

} task_class_t;

typedef enum
{
    TASK_TYPE_LED,
    TASK_TYPE_I2C,
    TASK_TYPE_TAG_THERMOMETHER,
    TASK_TYPE_TAG_LUX_METER,
    TASK_TYPE_TAG_BAROMETER,
    TASK_TYPE_TAG_HUMIDITY,
    TASK_TYPE_TAG_NFC,
    TASK_TYPE_MODULE_RELAY,
    TASK_TYPE_MODULE_CO2,
    TASK_TYPE_DISPLAY_OLED

} task_type_t;

typedef struct
{
    bc_os_task_t task;
    bc_os_semaphore_t semaphore;

    bc_tick_t _tick_feed_interval;
    bc_tick_t _tick_last_feed;

    bc_bridge_t *_bridge;
    bc_bridge_i2c_channel_t _i2c_channel;
    uint8_t _device_address;

    bool _quit;
    bool init_done;

    bc_os_mutex_t *mutex;
    void *parameters;
    bc_tick_t *publish_interval;

} task_worker_t;

typedef struct
{
    task_class_t class;
    task_type_t type;
    bc_bridge_i2c_channel_t i2c_channel;
    uint8_t device_address;

    bc_os_mutex_t *mutex;

    bc_tick_t *publish_interval;
    void *parameters;

    task_worker_t *worker;


} task_info_t;

bool task_is_alive(task_info_t *task_info);
bool task_is_alive_and_init_done(task_info_t *task_info);
bool task_set_interval(task_info_t *task_info, bc_tick_t interval);
void task_get_interval(task_info_t *task_info, bc_tick_t *interval);
void task_lock(task_info_t *task_info);
void task_unlock(task_info_t *task_info);
void task_semaphore_put(task_info_t *task_info);

bool task_worker_is_quit_request(task_worker_t *self);
void task_worker_get_interval(task_worker_t *self, bc_tick_t *interval);
void task_worker_set_init_done(task_worker_t *self);

#endif //BC_BRIDGE_TASK_H
