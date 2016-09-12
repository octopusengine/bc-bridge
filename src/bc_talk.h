#ifndef _BC_TALK_H
#define _BC_TALK_H

#include "bc_common.h"

typedef enum
{
    BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL,
    BC_TALK_OPERATION_LED_SET,
    BC_TALK_OPERATION_LED_GET,
    BC_TALK_OPERATION_SET,
    BC_TALK_OPERATION_GET

} bc_talk_event_operation_t;

typedef struct
{
    bc_talk_event_operation_t operation;
    int value;
    uint8_t i2c_channel;
    uint8_t device_address;

} bc_talk_event_t;

void bc_talk_init(void);
void bc_talk_publish_begin(char *topic);
void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...);
void bc_talk_publish_add_value(char *name, char *value, ...);
void bc_talk_publish_end(void);

void bc_talk_publish_led_state(int state);

bool bc_talk_parse(char *line, size_t length, void (*callback)(bc_talk_event_t *event));

#endif /* _BC_TALK_H */
