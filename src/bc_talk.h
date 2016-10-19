#ifndef _BC_TALK_H
#define _BC_TALK_H

#include "bc_common.h"

#define BC_TALK_INT_VALUE_NULL -1
#define BC_TALK_INT_VALUE_INVALID -2

typedef enum
{
    BC_TALK_OPERATION_CONFIG_SET_PUBLISH_INTERVAL,
    BC_TALK_OPERATION_CONFIG_GET,
    BC_TALK_OPERATION_CONFIG_DEVICES_LIST,
    BC_TALK_OPERATION_LED_SET,
    BC_TALK_OPERATION_LED_GET,
    BC_TALK_OPERATION_RELAY_SET,
    BC_TALK_OPERATION_RELAY_GET,
    BC_TALK_OPERATION_LINE_SET,
    BC_TALK_OPERATION_GET,


} bc_talk_event_operation_t;

typedef struct
{
    bc_talk_event_operation_t operation;
    int param;
    void *value;
    uint8_t i2c_channel;
    uint8_t device_address;

} bc_talk_event_t;

typedef void (*bc_talk_parse_callback)(bc_talk_event_t *event);

void bc_talk_init(bc_talk_parse_callback callback);
void bc_talk_publish_begin(char *topic);
void bc_talk_publish_begin_auto(uint8_t i2c_channel, uint8_t device_address);
void bc_talk_publish_begin_auto_subtopic(uint8_t i2c_channel, uint8_t device_address, char *subtopic);

void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...);
void bc_talk_publish_add_value(char *name, char *value, ...);
void bc_talk_publish_end(void);

char *bc_talk_get_device_name(uint8_t device_address, char* output_str, size_t max_len );
void bc_talk_make_topic(uint8_t i2c_channel, uint8_t device_address, char *topic, size_t topic_size);

void bc_talk_publish_led_state(int state);
void bc_talk_publish_relay(int state, uint8_t device_address);

bool bc_talk_parse_start(char *line, size_t length);
bool bc_talk_parse(char *line, size_t length, bc_talk_parse_callback callback);

#endif /* _BC_TALK_H */
