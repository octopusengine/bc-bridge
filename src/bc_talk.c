#include "bc_talk.h"
#include "bc_os.h"
#include <jsmn.h>
#include "bc_log.h"

const char *bc_talk_led_state[] = { "off", "on", "1-dot", "2-dot", "3-dot" };
const char *bc_talk_bool[] = { "false", "true" };

static bc_os_mutex_t bc_talk_mutex;
static bool bc_talk_add_comma=false;

static bool _bc_talk_schema_check(int r, jsmntok_t *tokens);
static bool _bc_talk_token_cmp(char *line, jsmntok_t *tok, const char *s);
static int _bc_talk_token_get_int(char *line, jsmntok_t *tok);
static int _bc_talk_token_get_bool_as_int(char *line, jsmntok_t *tok);
static int _bc_talk_token_find_index(char *line, jsmntok_t *tok, const char *list[], size_t length);
static bool _bc_talk_set_i2c(char *str, bc_talk_event_t *event);

void bc_talk_init(void)
{
    bc_os_mutex_init(&bc_talk_mutex);
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

void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...)
{
    va_list ap;

    if(bc_talk_add_comma)
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

    if(bc_talk_add_comma)
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

    bc_os_mutex_unlock(&bc_talk_mutex);
}

char *bc_talk_get_device_name(uint8_t device_address)
{
    switch (device_address)
    {
        case 0x00: return "led";
        case 0x38: return "co2-sensor";
        case 0x3B: return "relay";
        case 0x44: return "lux-meter";
        case 0x45: return "lux-meter";
        case 0x48: return "thermometer";
        case 0x49: return "thermometer";
        case 0x5F: return "humidity-sensor";
        case 0x60: return "barometer";
        default: return "-";
    }
}

void bc_talk_make_topic(uint8_t i2c_channel, uint8_t device_address, char *topic, size_t length)
{
    if (device_address==0)
    {
        snprintf(topic, length, "%s/-", bc_talk_get_device_name(device_address));
    }
    else
    {
        snprintf(topic, length, "%s/i2c%d-%02x", bc_talk_get_device_name(device_address), (uint8_t) i2c_channel, device_address);
    }
}

void bc_talk_publish_led_state(int state)
{
    bc_talk_publish_begin("led/-");
    bc_talk_publish_add_value("state", "\"%s\"", ((state > -1) && (state < sizeof(bc_talk_led_state))) ? bc_talk_led_state[state] : "null" );
    bc_talk_publish_end();
}

void bc_talk_publish_relay(int state, uint8_t device_address)
{
    bc_talk_publish_begin_auto(0, device_address);
    switch (state)
    {
        case 1 :
        {
            bc_talk_publish_add_value("state", "%s", "true" );
            break;
        }
        case 0 :
        {
            bc_talk_publish_add_value("state", "%s", "false" );
            break;
        }
        default:
            bc_talk_publish_add_value("state", "%s", "null" );
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

    return _bc_talk_token_cmp(line, &tokens[1], "$config/clown.talk/-/create");

}

bool bc_talk_parse(char *line, size_t length, void (*callback)(bc_talk_event_t *event))
{
    jsmn_parser parser;
    jsmntok_t tokens[20];
    int r;
    int i;
    char *payload_string;
    char *payload[10];
    int payload_length = 0;
    char *split;
    char *saveptr;
    bc_talk_event_t event;

    jsmn_init(&parser);
    r = jsmn_parse(&parser, line, length, tokens, sizeof(tokens) );

    if (!_bc_talk_schema_check(r, tokens))
    {
        return false;
    }

    payload_string = malloc( sizeof(char) * (tokens[1].end-tokens[1].start+1) );

    if (payload_string == NULL)
    {
        bc_log_fatal("bc_talk_parse: call failed: malloc");
    }

    strncpy(payload_string, line+tokens[1].start, tokens[1].end-tokens[1].start );
    payload_string[ tokens[1].end-tokens[1].start ] = 0x00;

    bc_log_debug("bc_talk_parse: payload %s", payload_string);

    split = strtok_r(payload_string, "/", &saveptr);
    while (split)
    {
        payload[payload_length++] = strdup(split);
        split = strtok_r(0, "/", &saveptr);
    }

    if ((strcmp(payload[0], "$config") == 0) && (payload_length == 5) && (strcmp(payload[1], "devices") == 0) )
    {

        if ((strcmp(payload[payload_length-1], "list") == 0) && (r==3) && (strcmp(payload[2], "-") == 0) && (strcmp(payload[3], "-") == 0) )
        {
            event.operation = BC_TALK_OPERATION_CONFIG_LIST;
            callback(&event);
            return true;
        }

        if (!_bc_talk_set_i2c(payload[payload_length-2], &event))
        {
            bc_log_error("bc_talk_parse: bad i2c address");
            return false;
        }

        if (strcmp(payload[2], bc_talk_get_device_name(event.device_address)) != 0)
        {
            bc_log_error("bc_talk_parse: bad payload: contained %s expected %s", payload[2], bc_talk_get_device_name(event.device_address) );
            return false;
        }

        if (strcmp(payload[payload_length-1], "update") == 0)
        {
            for (i=3; i<tokens[2].size*2+3 && i+1<r; i+=2)
            {
                if (_bc_talk_token_cmp(line, &tokens[i], "publish-interval")){

                    event.operation = BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL;
                    event.value = _bc_talk_token_get_int(line, &tokens[i+1] );
                    if (event.value != BC_TALK_INT_VALUE_INVALID)
                    {
                        callback(&event);
                    }
                }
            }
        }
        else if (strcmp(payload[payload_length-1], "read") == 0)
        {
            event.operation = BC_TALK_OPERATION_CONFIG_READ;
            callback(&event);
        }
    }

    else if ((strcmp(payload[0], "led") == 0) && (payload_length == 3) )
    {
        event.i2c_channel = 0;
        event.device_address = 0;
        if ((strcmp(payload[2], "set") == 0) && _bc_talk_token_cmp(line, &tokens[3], "state") )
        {
            event.operation = BC_TALK_OPERATION_LED_SET;
            event.value = _bc_talk_token_find_index(line, &tokens[4], bc_talk_led_state, sizeof(bc_talk_led_state)/sizeof(*bc_talk_led_state) );
            if (event.value != BC_TALK_INT_VALUE_INVALID)
            {
                callback(&event);
            }
        }
        else if ((strcmp(payload[2], "get") == 0) )
        {
            event.operation = BC_TALK_OPERATION_LED_GET;
            callback(&event);
        }

    }else if ((strcmp(payload[0], "relay") == 0) && (payload_length == 3) )
    {
        if (!_bc_talk_set_i2c(payload[payload_length-2], &event))
        {
            bc_log_error("bc_talk_parse: bad i2c address");
            return false;
        }
        if ((strcmp(payload[2], "set") == 0) && _bc_talk_token_cmp(line, &tokens[3], "state") )
        {
            event.operation = BC_TALK_OPERATION_RELAY_SET;
            event.value = _bc_talk_token_get_bool_as_int(line, &tokens[4]);
            if (event.value != BC_TALK_INT_VALUE_INVALID)
            {
                callback(&event);
            }
        }
        else if ((strcmp(payload[2], "get") == 0) )
        {
            event.operation = BC_TALK_OPERATION_RELAY_GET;
            callback(&event);
        }

    }

    free(payload_string);
    return true;
}

static bool _bc_talk_schema_check(int r, jsmntok_t *tokens)
{
    if (r < 0) {
        bc_log_error("bc_talk_parse: Failed to parse JSON: %d", r);
        return false;
    }

    if (r < 1 || tokens[0].type != JSMN_ARRAY) {
        bc_log_error("bc_talk_parser: Array expected");
        return false;
    }

    if (tokens[0].size > 2) {
        bc_log_error("bc_talk_parser: too big Array");
        return false;
    }

    if (r < 2 || tokens[1].type != JSMN_STRING) {
        bc_log_error("bc_talk_parse: payload: String expected");
        return false;
    }

    if (r < 3 || tokens[2].type != JSMN_OBJECT) {
        bc_log_error("bc_talk_parse: Object expected");
        return false;
    }

    return true;
}

static bool _bc_talk_token_cmp(char *line, jsmntok_t *tok, const char *s) {
    if ((tok->type == JSMN_STRING) && ((int) strlen(s) == tok->end - tok->start) &&
            (strncmp(line + tok->start, s, tok->end - tok->start) == 0))
    {
        return true;
    }
    return false;
}


static int _bc_talk_token_get_int(char *line, jsmntok_t *tok)
{
    char temp[10];
    int ret;

    if (tok->type != JSMN_PRIMITIVE)
    {
        return BC_TALK_INT_VALUE_INVALID;
    }

    memset(temp, 0x00, sizeof(temp));
    strncpy(temp, line+tok->start, tok->end - tok->start < sizeof(temp) ? tok->end - tok->start : sizeof(temp) - 1 );

    if (strcmp(temp, "null") == 0)
    {
        return BC_TALK_INT_VALUE_NULL;
    }

    if (strchr(temp, 'e')) //support 1e3
    {
        ret = (int)strtof(temp, NULL);
    }
    else
    {
        ret = (int)strtol(temp, NULL, 10);
    }

    if (ret < 0 )
    {
        return BC_TALK_INT_VALUE_INVALID;
    }
    return ret;
}

static int _bc_talk_token_get_bool_as_int(char *line, jsmntok_t *tok)
{
    if (tok->type != JSMN_PRIMITIVE)
    {
        return BC_TALK_INT_VALUE_INVALID;
    }
    return _bc_talk_token_find_index(line, tok, bc_talk_bool, sizeof(bc_talk_bool)/sizeof(*bc_talk_bool) );
}

static int _bc_talk_token_find_index(char *line, jsmntok_t *tok,const char *list[], size_t length){
    char *temp;
    int i;
    int temp_length = tok->end - tok->start;
    temp = malloc(sizeof(char) * (temp_length + 1) );

    if (temp == NULL)
    {
        bc_log_fatal("_bc_talk_token_find_index: call failed: malloc");
    }

    temp[ temp_length ] = 0x00;
    strncpy(temp, line+tok->start, temp_length);

    for (i=0; i<length; i++)
    {
        if (strcmp(list[i], temp) == 0)
        {
            free(temp);
            return i;
        }
    }
    free(temp);
    return BC_TALK_INT_VALUE_INVALID;
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

static bool _bc_talk_set_i2c(char *str, bc_talk_event_t *event)
{
    char * pEnd;
    if (strlen(str) != 7)
    {
        if (strcmp(str, "-") == 0)
        {
            event->i2c_channel = 0;
            event->device_address = 0;
            return true;
        }
        return false;
    }
    event->i2c_channel = (uint8_t)strtol(str+3, &pEnd, 10);
    event->device_address = (uint8_t)strtol(str+5, &pEnd, 16);

    return true;
}
