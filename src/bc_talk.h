#ifndef _BC_TALK_H
#define _BC_TALK_H

#include "bc_common.h"

#define BC_TALK_UINT_VALUE_NULL -1
#define BC_TALK_UINT_VALUE_INVALID -2

#define BC_TALK_LED_ADDRESS  0x80
#define BC_TALK_I2C_ADDRESS  0x81
#define BC_TALK_UART_ADDRESS 0x82

#define BC_TALK_DEVICE_NAME_SIZE 21

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
    BC_TALK_OPERATION_RAW_SET,
    BC_TALK_OPERATION_GET,
    BC_TALK_OPERATION_I2C_SCAN,
    BC_TALK_OPERATION_I2C_WRITE,
    BC_TALK_OPERATION_I2C_READ,

} bc_talk_event_operation_t;

typedef enum
{
    BC_TALK_DATA_ENCODING_NULL = 0,
    BC_TALK_DATA_ENCODING_HEX,
    BC_TALK_DATA_ENCODING_ASCII,
    BC_TALK_DATA_ENCODING_BASE64

} bc_talk_data_encoding_t;

typedef struct
{
    uint8_t *buffer;
    size_t length;
    bc_talk_data_encoding_t encoding;

} bc_talk_data_t;

typedef struct
{
    uint8_t channel;
    uint8_t device_address;
    bc_talk_data_t write;
    bc_talk_data_t read;
    uint8_t read_length;

} bc_talk_i2c_attributes_t;

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

bool bc_talk_is_clown_device(uint8_t device_address);
char *bc_talk_get_device_name(uint8_t device_address, char *output_str, size_t max_len);
void bc_talk_make_topic(uint8_t i2c_channel, uint8_t device_address, char *topic, size_t topic_size);

void bc_talk_publish_led_state(int state);
void bc_talk_publish_relay(int state, uint8_t device_address);
void bc_talk_publish_i2c(bc_talk_i2c_attributes_t *attributes);

void bc_talk_i2c_attributes_destroy(bc_talk_i2c_attributes_t *attributes);

bool bc_talk_parse_start(char *line, size_t length);
bool bc_talk_parse(char *line, size_t length, bc_talk_parse_callback callback);

#endif /* _BC_TALK_H */
