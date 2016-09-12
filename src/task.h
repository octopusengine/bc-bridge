#ifndef BC_BRIDGE_TASK_H
#define BC_BRIDGE_TASK_H

typedef enum {
    TASK_CLASS_SENSOR,
    TASK_CLASS_ACTUATOR,
    TASK_CLASS_BRIDGE

} task_class_t;

typedef enum {
    TAG_THERMOMETHER,
    TAG_LUX_METER,
    MODULE_RELAY,
    MODULE_CO2,
    TASK_TYPE_LED

} task_type_t;

typedef struct {
    task_class_t class;
    task_type_t type;
    bc_bridge_i2c_channel_t i2c_channel;
    uint8_t device_address;
    void *task;
    bool enabled;

} task_info_t;

void task_init(bc_bridge_t *bridge, task_info_t *task_info_list, size_t length);
void task_set_interval(task_info_t *task_info, bc_tick_t interval);

#endif //BC_BRIDGE_TASK_H
