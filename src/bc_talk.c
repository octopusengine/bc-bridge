#include "bc_talk.h"
#include "bc_os.h"
#include <jsmn.h>
#include "bc_log.h"
#include "bc_base64.h"

#define BC_TALK_MAX_PAYLOAD_SUBTOPIC 4

#define BC_TALK_RAW_BASE64_LENGTH 1368
#define BC_TALK_RAW_BUFFER_LENGTH 1024

static char *bc_talk_device_names[256];

const char *bc_talk_led_state[] = { "off", "on", "1-dot", "2-dot", "3-dot" };
const char *bc_talk_bool[] = { "false", "true" };
const char *bc_talk_lines[] = { "line-0", "line-1", "line-2", "line-3", "line-4", "line-5", "line-6", "line-7", "line-8" };

const  char bc_talk_on_escape[] = {'\\','"','/','\b','\f','\n','\r','\t'};
const  char bc_talk_escape[] = {'\\','"','/','b','f','n','r','t'};


static bc_os_mutex_t bc_talk_mutex;
static bc_os_task_t bc_talk_task_stdin;
static bool bc_talk_add_comma = false;
static bc_talk_parse_callback bc_talk_callback = NULL;

static bool _bc_talk_schema_check(int r, jsmntok_t *tokens);
static bool _bc_talk_token_cmp(char *line, jsmntok_t *tok, const char *s);
static char *_bc_talk_token_get_string(char *line, jsmntok_t *tok, char* output_str, size_t max_len);
static int _bc_talk_token_get_uint(char *line, jsmntok_t *tok);
static int _bc_talk_token_get_bool_as_int(char *line, jsmntok_t *tok);
static int _bc_talk_token_find_index(char *line, jsmntok_t *tok, const char *list[], size_t length);
static bool _bc_talk_set_i2c(char *str0, char *str1, bc_talk_event_t *event);
static void *bc_talk_worker_stdin(void *parameter);
void _bc_talk_token_get_data(char *line, jsmntok_t *tok, bc_talk_data_t *data);
static char *_bc_talk_data_to_string(bc_talk_data_t *data);

#define BC_TALK_FREE_PAYLOAD for (i=0; i<payload_length; i++) { free(payload[i]); }

void bc_talk_init(bc_talk_parse_callback callback)
{

    if (callback == NULL)
    {
        bc_log_fatal("task_thermometer_spawn: callback == NULL");
    }

    bc_talk_callback = callback;

    memset(bc_talk_device_names, 0, sizeof(bc_talk_device_names));

    bc_talk_device_names[0x38] = "co2-sensor";
    bc_talk_device_names[0x3B] = "relay";
    bc_talk_device_names[0x3C] = "display-oled";
    bc_talk_device_names[0x3F] = "relay";
    bc_talk_device_names[0x44] = "lux-meter";
    bc_talk_device_names[0x45] = "lux-meter";
    bc_talk_device_names[0x48] = "thermometer";
    bc_talk_device_names[0x49] = "thermometer";
    bc_talk_device_names[0x4D] = "!co2-sensor-i2c-uart";
    bc_talk_device_names[0x5F] = "humidity-sensor";
    bc_talk_device_names[0x60] = "barometer";
    bc_talk_device_names[0x70] = "!i2c-selector";
    bc_talk_device_names[BC_TALK_LED_ADDRESS] = "led";
    bc_talk_device_names[BC_TALK_I2C_ADDRESS] = "i2c";
    bc_talk_device_names[BC_TALK_UART_ADDRESS] = "uart";

    bc_os_mutex_init(&bc_talk_mutex);
    bc_os_task_init(&bc_talk_task_stdin, bc_talk_worker_stdin, NULL);
}

void bc_talk_publish_begin(char *topic)
{
    bc_os_mutex_lock(&bc_talk_mutex);
    bc_talk_add_comma = false;
    fprintf(stdout, "[\"%s\", {", topic);
}

void bc_talk_publish_begin_auto(uint8_t i2c_channel, uint8_t device_address)
{
    char topic[64];
    bc_talk_make_topic(i2c_channel, device_address, topic, sizeof(topic));
    bc_talk_publish_begin(topic);
}

void bc_talk_publish_begin_auto_subtopic(uint8_t i2c_channel, uint8_t device_address, char *subtopic)
{
    char topic[64];
    bc_talk_make_topic(i2c_channel, device_address, topic, sizeof(topic));
    strcat(topic, subtopic);
    bc_talk_publish_begin(topic);
}


void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...)
{
    va_list ap;

    if (bc_talk_add_comma)
    {
        fprintf(stdout, ", ");
    }

    fprintf(stdout, "\"%s\": [", name);


    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    fprintf(stdout, ", \"%s\"]", unit);

    bc_talk_add_comma = true;

}

void bc_talk_publish_add_value(char *name, char *value, ...)
{
    va_list ap;

    if (bc_talk_add_comma)
    {
        fprintf(stdout, ", ");
    }

    fprintf(stdout, "\"%s\": ", name);

    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    bc_talk_add_comma = true;

}

void bc_talk_publish_end(void)
{
    fprintf(stdout, "}]\n");
    fflush(stdout);

    bc_os_mutex_unlock(&bc_talk_mutex);
}

bool bc_talk_is_clown_device(uint8_t device_address)
{
    return bc_talk_device_names[device_address] != NULL;
}

char *bc_talk_get_device_name(uint8_t device_address, char* output_str, size_t max_len )
{
    char *str;

    if (bc_talk_device_names[device_address] == NULL)
    {
        str = "-";
    }
    else
    {
        str = bc_talk_device_names[device_address];
    }

    size_t l = strlen(str);
    if (l+1 > max_len) {
        return NULL;
    }

    strcpy(output_str, str);
    return output_str;
}

void bc_talk_make_topic(uint8_t i2c_channel, uint8_t device_address, char *topic, size_t topic_size)
{
    char name[BC_TALK_DEVICE_NAME_SIZE];

    bc_talk_get_device_name(device_address, name, BC_TALK_DEVICE_NAME_SIZE );

    if (name[0] == '!')
    {
        strcpy(topic, name);
    }
    else if (device_address >= 0x80)
    {
        if (device_address == BC_TALK_I2C_ADDRESS)
        {
            snprintf(topic, topic_size, "%s/%d", name, i2c_channel );
        }
        else
        {
            snprintf(topic, topic_size, "%s/-", name );
        }
    }
    else
    {
        snprintf(topic, topic_size, "%s/i2c%d-%02x", name, (uint8_t) i2c_channel,
                 device_address);
    }
}

void bc_talk_publish_led_state(int state)
{
    bc_talk_publish_begin("led/-");
    bc_talk_publish_add_value("state", "\"%s\"",
                              ((state > -1) && (state < sizeof(bc_talk_led_state))) ? bc_talk_led_state[state]
                                                                                    : "null");
    bc_talk_publish_end();
}

void bc_talk_publish_relay(int state, uint8_t device_address)
{
    bc_talk_publish_begin_auto(0, device_address);
    switch (state)
    {
        case 1 :
        {
            bc_talk_publish_add_value("state", "%s", "true");
            break;
        }
        case 0 :
        {
            bc_talk_publish_add_value("state", "%s", "false");
            break;
        }
        default:
            bc_talk_publish_add_value("state", "%s", "null");
    }
    bc_talk_publish_end();
}

void bc_talk_publish_i2c(bc_talk_i2c_attributes_t *attributes)
{
    bc_talk_publish_begin_auto(attributes->channel, BC_TALK_I2C_ADDRESS);

    bc_talk_publish_add_value("address", "\"%02X\"", attributes->device_address);

    char *data_string;

    if ( attributes->write.buffer != NULL)
    {
        data_string = _bc_talk_data_to_string(&attributes->write);
        if (data_string != NULL)
        {
            bc_talk_publish_add_value("write", "\"%s\"", data_string);
            free(data_string);
        }
    }

    if ( attributes->read.buffer != NULL)
    {
        data_string = _bc_talk_data_to_string(&attributes->read);
        if (data_string != NULL)
        {
            bc_talk_publish_add_value("read", "\"%s\"", data_string);
            free(data_string);
        }
    }

    bc_talk_publish_end();
}

bool bc_talk_parse_start(char *line, size_t length)
{
    jsmn_parser parser;
    jsmntok_t tokens[20];
    int r;

    jsmn_init(&parser);
    r = jsmn_parse(&parser, line, length, tokens, sizeof(tokens));

    if (!_bc_talk_schema_check(r, tokens))
    {
        return false;
    }

    return _bc_talk_token_cmp(line, &tokens[1], "clown.talk/-/config/set" );

}

bool bc_talk_parse(char *line, size_t length, bc_talk_parse_callback callback)
{
    jsmn_parser parser;
    jsmntok_t tokens[20];
    int r;
    int i;
    char *payload_string;
    char payload[BC_TALK_MAX_PAYLOAD_SUBTOPIC][32];
    int payload_length = 0;
    char *split;
    char *saveptr;
    char *text_tmp;
    bc_talk_event_t event;
    event.value = NULL;

    jsmn_init(&parser);
    r = jsmn_parse(&parser, line, length, tokens, sizeof(tokens));

    if (!_bc_talk_schema_check(r, tokens))
    {
        return false;
    }

    payload_string = malloc(sizeof(char) * (tokens[1].end - tokens[1].start + 1));

    if (payload_string == NULL)
    {
        bc_log_fatal("bc_talk_parse: call failed: malloc");
    }

    strncpy(payload_string, line + tokens[1].start, (size_t)(tokens[1].end - tokens[1].start));
    payload_string[tokens[1].end - tokens[1].start] = 0x00;

    bc_log_debug("bc_talk_parse: payload %s", payload_string);

    split = strtok_r(payload_string, "/", &saveptr);
    while (split && (payload_length <= BC_TALK_MAX_PAYLOAD_SUBTOPIC))
    {
        if (strlen(split) > 32)
        {
            bc_log_error("bc_talk_parse: too long subtopic");
            return false;
        }
        strcpy(payload[payload_length++], split);
        split = strtok_r(0, "/", &saveptr);
    }
    free(payload_string);

    if (payload_length > BC_TALK_MAX_PAYLOAD_SUBTOPIC)
    {
        bc_log_error("bc_talk_parse: too many subtopic");
        return false;
    }

    if ((payload_length<3))
    {
        bc_log_error("bc_talk_parse: short topic");
        return false;
    }

    if (!_bc_talk_set_i2c(payload[0], payload[1], &event))
    {
        bc_log_error("bc_talk_parse: bad i2c address");
        return false;
    }

    if ((strcmp(payload[2], "config") == 0) && (payload_length == 4) )
    {

        if ((strcmp(payload[3], "list") == 0) && (r == 3) && (event.i2c_channel == 0) && (event.device_address == 0))
        {
            event.operation = BC_TALK_OPERATION_CONFIG_DEVICES_LIST;
            callback(&event);
            return true;
        }

        if ((event.device_address == BC_TALK_I2C_ADDRESS) && (strcmp(payload[3], "scan") == 0) && (r == 3) )
        {
                event.operation = BC_TALK_OPERATION_I2C_SCAN;

                if (strcmp(payload[1], "0") == 0)
                {
                    event.param = 0;
                }
                else if (strcmp(payload[1], "1") == 0)
                {
                    event.param = 1;
                }
                else
                {
                    bc_log_error("bc_talk_parse: bad i2c bus");
                    return false;
                }

                callback(&event);
                return true;
        }

        text_tmp = malloc(BC_TALK_DEVICE_NAME_SIZE*sizeof(char));
        if (strcmp(payload[0], bc_talk_get_device_name(event.device_address, text_tmp, BC_TALK_DEVICE_NAME_SIZE )) != 0)
        {
            bc_log_error("bc_talk_parse: bad payload: contained %s expected %s", payload[2], text_tmp);
            return false;
        }
        free(text_tmp);

        if (strcmp(payload[payload_length - 1], "update") == 0)
        {

            for (i = 3; i < tokens[2].size * 2 + 3 && i + 1 < r; i += 2)
            {
                if (_bc_talk_token_cmp(line, &tokens[i], "publish-interval"))
                {
                    event.operation = BC_TALK_OPERATION_CONFIG_SET_PUBLISH_INTERVAL;
                    event.param = _bc_talk_token_get_uint(line, &tokens[i + 1]);
                    if (event.param != BC_TALK_UINT_VALUE_INVALID)
                    {
                        callback(&event);
                    }
                }
            }
        }
        else if (strcmp(payload[payload_length - 1], "get") == 0)
        {
            event.operation = BC_TALK_OPERATION_CONFIG_GET;
            callback(&event);
        }
        else if (strcmp(payload[payload_length - 1], "list") == 0)
        {
            event.operation = BC_TALK_OPERATION_CONFIG_GET;
            callback(&event);
        }
        return true;
    }

    text_tmp = malloc(BC_TALK_DEVICE_NAME_SIZE*sizeof(char));
    if (strcmp(payload[0], bc_talk_get_device_name(event.device_address, text_tmp, BC_TALK_DEVICE_NAME_SIZE )) != 0)
    {
        bc_log_error("bc_talk_parse: bad payload: contained %s expected %s", payload[2], text_tmp);
        return false;
    }
    free(text_tmp);

    if ((strcmp(payload[0], "led") == 0) && (payload_length == 3))
    {
        if ((strcmp(payload[2], "set") == 0) && _bc_talk_token_cmp(line, &tokens[3], "state"))
        {
            event.operation = BC_TALK_OPERATION_LED_SET;
            event.param = _bc_talk_token_find_index(line, &tokens[4], bc_talk_led_state,
                                                    sizeof(bc_talk_led_state) / sizeof(*bc_talk_led_state));
            if (event.param != BC_TALK_UINT_VALUE_INVALID)
            {
                callback(&event);
            }
        }
        else if ((strcmp(payload[2], "get") == 0))
        {
            event.operation = BC_TALK_OPERATION_LED_GET;
            callback(&event);
        }

    }
    else if ((strcmp(payload[0], "relay") == 0) && (payload_length == 3))
    {
        if ((strcmp(payload[2], "set") == 0) && _bc_talk_token_cmp(line, &tokens[3], "state"))
        {
            event.operation = BC_TALK_OPERATION_RELAY_SET;
            event.param = _bc_talk_token_get_bool_as_int(line, &tokens[4]);
            if (event.param != BC_TALK_UINT_VALUE_INVALID)
            {
                callback(&event);
            }
        }
        else if ((strcmp(payload[2], "get") == 0))
        {
            event.operation = BC_TALK_OPERATION_RELAY_GET;
            callback(&event);
        }

    }
    else if ((strcmp(payload[0], "display-oled") == 0) && (payload_length == 3))
    {
        if (strcmp(payload[2], "set") == 0)
        {

            if (_bc_talk_token_cmp(line, &tokens[3], "raw") && (r==5))
            {
                event.operation = BC_TALK_OPERATION_RAW_SET;

                text_tmp = malloc(sizeof(char)*BC_TALK_RAW_BASE64_LENGTH);
                uint8_t *buffer;
                size_t buffer_length = sizeof(uint8_t)*BC_TALK_RAW_BUFFER_LENGTH;
                buffer = malloc(buffer_length);

                _bc_talk_token_get_string(line, &tokens[4], text_tmp, BC_TALK_RAW_BASE64_LENGTH );
                bc_log_debug("base %s", text_tmp);

                if (bc_base64_decode(buffer, &buffer_length, text_tmp, BC_TALK_RAW_BASE64_LENGTH ))
                {
                    event.value = buffer;
                }

                free(text_tmp);

                if (event.value!=NULL)
                {
                    callback(&event);
                }

            }
            else
            {
                event.operation = BC_TALK_OPERATION_LINE_SET;
                text_tmp = malloc(sizeof(char)*22);
                for (i = 3; i < tokens[2].size * 2 + 3 && i + 1 < r; i += 2)
                {
                    event.param = _bc_talk_token_find_index(line, &tokens[i], bc_talk_lines,
                                                            sizeof(bc_talk_lines) / sizeof(*bc_talk_lines));
                    if ( event.param > -1 ){

                        event.value = _bc_talk_token_get_string(line, &tokens[i + 1], text_tmp, 22 );
                        if (event.value != NULL)
                        {
                            callback(&event);
                        }
                        else
                        {
                            bc_log_error("bc_talk_parse: bad length max 21 char");
                        }
                    }
                }
                free(text_tmp);
            }
        }
    }
    else if (strcmp(payload[0], "i2c") == 0)
    {

        bc_talk_i2c_attributes_t *i2c_attributes = malloc(sizeof(bc_talk_i2c_attributes_t));
        memset(i2c_attributes, 0, sizeof(bc_talk_i2c_attributes_t));

        if (strcmp(payload[1], "0") == 0)
        {
            i2c_attributes->channel = 0;
        }
        else if (strcmp(payload[1], "1") == 0)
        {
            i2c_attributes->channel = 1;
        }
        else
        {
            bc_log_error("bc_talk_parse: bad i2c bus");
            return false;
        }

        for (i = 3; i < tokens[2].size * 2 + 3 && i + 1 < r; i += 2)
        {
            if (_bc_talk_token_cmp(line, &tokens[i], "address") && (tokens[i+1].type == JSMN_STRING))
            {

                i2c_attributes->device_address = 128;
                i2c_attributes->device_address = (uint8_t) strtol(line+tokens[i+1].start, NULL, 16);
                if (i2c_attributes->device_address > 127)
                {
                    bc_log_error("bc_talk_parse: bad slave address");

                    return false;
                }

            }
            else if (_bc_talk_token_cmp(line, &tokens[i], "write") && (tokens[i+1].type == JSMN_STRING))
            {
                _bc_talk_token_get_data(line, &tokens[i+1], &i2c_attributes->write );

                if (i2c_attributes->write.encoding == BC_TALK_DATA_ENCODING_NULL)
                {
                    bc_log_error("bc_talk_parse: parse write data");

                    return false;
                }
            }
            else if (_bc_talk_token_cmp(line, &tokens[i], "read-length") && (tokens[i+1].type == JSMN_PRIMITIVE))
            {
                i2c_attributes->read_length = _bc_talk_token_get_uint(line, &tokens[i+1]);
            }
        }

        if (strcmp(payload[2], "set") == 0)
        {
            event.operation = BC_TALK_OPERATION_I2C_WRITE;

            if (i2c_attributes->read_length > 0)
            {
                event.operation = BC_TALK_OPERATION_I2C_READ;
            }
        }

        if (strcmp(payload[2], "get") == 0)
        {
            event.operation = BC_TALK_OPERATION_I2C_READ;

            if (i2c_attributes->read_length < 1)
            {
                bc_log_error("bc_talk_parse: read-length must be positive number");

                return false;
            }

        }

        if ((event.operation == BC_TALK_OPERATION_I2C_READ) && (i2c_attributes->write.length > 2) )
        {
            bc_log_error("bc_talk_parse: for read register, attribute write max length of 2 bytes");

            return false;
        }

        event.value = i2c_attributes;
        callback(&event);

    }
    else if ((strcmp(payload[2], "get") == 0))
    {
        event.operation = BC_TALK_OPERATION_GET;
        callback(&event);
    }

    return true;
}

static bool _bc_talk_schema_check(int r, jsmntok_t *tokens)
{
    if (r < 0)
    {
        bc_log_error("bc_talk_parse: Failed to parse JSON: %d", r);
        return false;
    }

    if (r < 1 || tokens[0].type != JSMN_ARRAY)
    {
        bc_log_error("bc_talk_parser: Array expected");
        return false;
    }

    if (tokens[0].size > 2)
    {
        bc_log_error("bc_talk_parser: too big Array");
        return false;
    }

    if (r < 2 || tokens[1].type != JSMN_STRING)
    {
        bc_log_error("bc_talk_parse: payload: String expected");
        return false;
    }

    if (r < 3 || tokens[2].type != JSMN_OBJECT)
    {
        bc_log_error("bc_talk_parse: Object expected");
        return false;
    }

    return true;
}

static bool _bc_talk_token_cmp(char *line, jsmntok_t *tok, const char *s)
{
    if ((tok->type == JSMN_STRING) && ((int) strlen(s) == tok->end - tok->start) &&
        (strncmp(line + tok->start, s, (size_t)(tok->end - tok->start)) == 0))
    {
        return true;
    }
    return false;
}

static char *_bc_talk_token_get_string(char *line, jsmntok_t *tok, char* output_str, size_t max_len)
{
    size_t l = (size_t) (tok->end - tok->start);
    if (l > max_len) {
        return NULL;
    }
    strncpy(output_str, line+tok->start, l);

    if (max_len > l)
    {
        output_str[l] = 0x00;
    }

    return output_str;
}

static int _bc_talk_token_get_uint(char *line, jsmntok_t *tok)
{
    char temp[10];
    int ret;

    if (tok->type != JSMN_PRIMITIVE)
    {
        return BC_TALK_UINT_VALUE_INVALID;
    }

    _bc_talk_token_get_string(line, tok, temp, 10);

    if (strcmp(temp, "null") == 0)
    {
        return BC_TALK_UINT_VALUE_NULL;
    }

    if (strchr(temp, 'e')) //support 1e3
    {
        ret = (int) strtof(temp, NULL);
    }
    else
    {
        ret = (int) strtol(temp, NULL, 10);
    }

    if (ret < 0)
    {
        return BC_TALK_UINT_VALUE_INVALID;
    }
    return ret;
}

static int _bc_talk_token_get_bool_as_int(char *line, jsmntok_t *tok)
{
    if (tok->type != JSMN_PRIMITIVE)
    {
        return BC_TALK_UINT_VALUE_INVALID;
    }
    return _bc_talk_token_find_index(line, tok, bc_talk_bool, sizeof(bc_talk_bool) / sizeof(*bc_talk_bool));
}

static int _bc_talk_token_find_index(char *line, jsmntok_t *tok, const char *list[], size_t length)
{
    char *temp;
    int i;
    int temp_length = tok->end - tok->start;
    temp = malloc(sizeof(char) * (temp_length + 1));

    if (temp == NULL)
    {
        bc_log_fatal("_bc_talk_token_find_index: call failed: malloc");
    }

    temp[temp_length] = 0x00;
    strncpy(temp, line + tok->start, (size_t)temp_length);

    for (i = 0; i < length; i++)
    {
        if (strcmp(list[i], temp) == 0)
        {
            free(temp);
            return i;
        }
    }
    free(temp);
    return BC_TALK_UINT_VALUE_INVALID;
}

void _bc_talk_token_get_data(char *line, jsmntok_t *tok, bc_talk_data_t *data)
{
    data->length = 0;
    data->buffer = NULL;
    data->encoding = BC_TALK_DATA_ENCODING_NULL;
    size_t i;
    int j;

    if (tok->type != JSMN_STRING)
    {
        return;
    }

    size_t l = (size_t) (tok->end - tok->start);
    size_t length = 0;

    if (l < 2)
    {
        bc_log_error("_bc_talk_token_get_data: too short");
        return;
    }

    if (line[tok->start] == '(')
    {
        if (line[tok->end - 1] != ')' )
        {
            bc_log_error("_bc_talk_token_get_data: ascii expect )");
            return;
        }
        data->encoding = BC_TALK_DATA_ENCODING_ASCII;
        data->buffer = malloc((l-2)*sizeof(uint8_t));

        for (i = tok->start+1; i < (tok->end - 1); i++ )
        {
            if (line[i] == '\\')
            {
                i++;
                for (j = 0; j < sizeof(bc_talk_escape); j++)
                {
                    if (line[i] == bc_talk_escape[j])
                    {
                        data->buffer[length++] = bc_talk_on_escape[j];
                        break;
                    }
                }
            }
            else
            {
                data->buffer[length++] = line[i];
            }

        }

        if (length != (l-2))
        {
            data->buffer = realloc(data->buffer, length);
        }
        data->length = length;

    }
    else if ( (l < 3) || (line[tok->start+2] == ',') )
    {
        data->encoding = BC_TALK_DATA_ENCODING_HEX;
        data->length = (l+1) / 3;
        data->buffer = malloc(data->length*sizeof(uint8_t));

        char hex[3] = {0,0,0};
        i = (size_t) tok->start;

        while ( length < data->length )
        {
            hex[0] = line[i];
            hex[1] = line[i+1];
            data->buffer[length++] = (uint8_t) strtol(hex, NULL, 16);

            if (i+3 > tok->end)
            {
                break;
            }

            if (line[i+2] != ',')
            {
                bc_log_error("_bc_talk_token_get_data: hex encoding expect delimiter , get %c", line[i+2]);
                break;
            }

            i += 3;
        }

    }
    else
    {
        data->encoding = BC_TALK_DATA_ENCODING_BASE64;
        data->length = bc_base64_calculate_decode_length(line+tok->start, l);
        data->buffer = malloc(data->length*sizeof(uint8_t));
        length = data->length;

        if (!bc_base64_decode(data->buffer, &length, line+tok->start, l))
        {
            data->length = 0;
        }

    }

    if (length != data->length)
    {
        data->length = 0;
        free(data->buffer);
        data->buffer = NULL;
    }

}

static char *_bc_talk_data_to_string(bc_talk_data_t *data)
{
    char *text=NULL;
    size_t length;
    size_t i;
    int j;

    if (data->length == 0)
    {
        text = malloc(1);
        text[0] = 0x00;
        return text;
    }

    if (data->encoding == BC_TALK_DATA_ENCODING_ASCII)
    {

        int escape = 0; // "
        for(i = 0; i < data->length; i++)
        {

            for (j = 0; j < sizeof(bc_talk_on_escape); j++)
            {
                if (data->buffer[i] == bc_talk_on_escape[j])
                {
                    escape++;
                    break;
                }
            }

            if ( (j == sizeof(bc_talk_on_escape) ) && ( (data->buffer[i] > 127) || (data->buffer[i] < 32) ) )
            {
                data->encoding = BC_TALK_DATA_ENCODING_HEX;
                break;
            }

        }

        if (data->encoding == BC_TALK_DATA_ENCODING_ASCII)
        {
            length = data->length + escape + 3;
            text = malloc(length);
            text[0] = '(';
            int offset = 1;
            for(i = 0; i < data->length; i++)
            {
                for (j = 0; j < sizeof(bc_talk_on_escape); j++ )
                {
                    if (data->buffer[i] == bc_talk_on_escape[j])
                    {
                        break;
                    }
                }

                if (j < sizeof(bc_talk_on_escape)){
                    text[i+offset] = '\\';
                    offset++;
                    text[i+offset] = bc_talk_escape[j];
                }
                else
                {
                    text[i+offset] = (char)data->buffer[i];
                }

            }
            text[i+offset] = ')';
            text[i+offset+1] = 0x00;
        }
    }

    if (data->encoding == BC_TALK_DATA_ENCODING_HEX)
    {
        length = data->length * 3;
        text = malloc(length);
        char tmp[4];

        text[0] = 0x00;

        for(i=0; i < data->length-1; i++)
        {
            snprintf(tmp, 4, "%02X,", data->buffer[i]);
            strcat(text, tmp);
        }

        snprintf(tmp, 4, "%02X", data->buffer[i]);
        strcat(text, tmp);
    }

    if (data->encoding == BC_TALK_DATA_ENCODING_BASE64)
    {
        length = bc_base64_calculate_encode_length(data->length);
        text = malloc(length);
        if (!bc_base64_encode(text, &length, data->buffer, data->length))
        {
            free(text);
            text = NULL;
        }
    }

    return text;
}

//static int _bc_talk_token_get_enum(char *line, jsmntok_t *tok, ...){
//
//    char temp[10];
//    char *str;
//    int i;
//    memset(temp, 0x00, sizeof(temp));
//    strncpy(&temp, line+tok->start, tok->end - tok->start < sizeof(temp) ? tok->end - tok->start : sizeof(temp) - 1 );
//
//    va_list vl;
//    va_start(vl,tok);
//    str=va_arg(vl,char*);
//    while (str!=NULL)
//    {
//        if (strcmp(str, temp) == 0)
//        {
//            return i;
//        }
//        str=va_arg(vl,char*);
//        i++;
//    }
//    va_end(vl);
//
//    return -1;
//}

static bool _bc_talk_set_i2c(char *str0, char *str1, bc_talk_event_t *event)
{
    char *pEnd;
    if (strlen(str1) != 7)
    {
        event->i2c_channel = 0;

        if (strcmp(str1, "-") == 0)
        {
            if (strcmp(str0, "-") == 0)
            {
                event->device_address = 0;
                return true;
            }
            else if (strcmp(str0, "led") == 0)
            {
                event->device_address = BC_TALK_LED_ADDRESS;
                return true;
            }
            return false;
        }
        else if (strcmp(str0, "i2c") == 0)
        {
            event->device_address = BC_TALK_I2C_ADDRESS;
            return true;
        }

        return false;
    }
    event->i2c_channel = (uint8_t) strtol(str1 + 3, &pEnd, 10);
    event->device_address = (uint8_t) strtol(str1 + 5, &pEnd, 16);

    return true;
}

static void *bc_talk_worker_stdin(void *parameter)
{

    char *line;
    size_t length;

    for (;;)
    {
        line = NULL;

        if (getline(&line, &length, stdin) != -1)
        {
            bc_talk_parse(line, length, bc_talk_callback );
            free(line);
        }
    }

    return NULL;
}
