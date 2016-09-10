#include "bc_talk.h"
#include "bc_os.h"
#include <jsmn.h>
#include "bc_log.h"

static bc_os_mutex_t bc_talk_mutex;
static bool bc_talk_add_comma;
static bool _bc_talk_token_cmp(char *json, jsmntok_t *tok, const char *s);
static int _bc_talk_token_get_int(char *line, jsmntok_t *tok);
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
    char temp[100];
    int temp_length;
    bc_talk_event_t event;

    jsmn_init(&parser);
    r = jsmn_parse(&parser, line, length, tokens, sizeof(tokens) );
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

    payload_string = malloc( sizeof(char) * (tokens[1].end-tokens[1].start+1) );
    strncpy(payload_string, line+tokens[1].start, tokens[1].end-tokens[1].start );
    payload_string[ tokens[1].end-tokens[1].start ] = 0x00;

    bc_log_debug("bc_talk_parse: payload %s", payload_string);

    split = strtok_r(payload_string, "/", &saveptr);
    while (split)
    {
        payload[payload_length++] = strdup(split);
        split = strtok_r(0, "/", &saveptr);
    }

    if ((strcmp(payload[0], "$config") == 0) && (payload_length > 2) )
    {

        if (!_bc_talk_set_i2c(payload[payload_length-2], &event))
        {
            bc_log_error("bc_talk_parse: bad i2c address");
            return false;
        }

        if (strcmp(payload[payload_length-1], "update") == 0)
        {
            for (i=3; i<tokens[2].size*2+3 && i+1<r; i+=2)
            {
                if (_bc_talk_token_cmp(line, &tokens[i], "publish-interval")){

                    event.operation = BC_TALK_OPERATION_UPDATE_PUBLISH_INTERVAL;
                    event.value = _bc_talk_token_get_int(line, &tokens[i+1] );
                    callback(&event);
                }
            }
        }
    }
    else if ((strcmp(payload[0], "led") == 0) && (payload_length > 2) )
    {
        event.i2c_channel = 0;
        event.device_address = 0;
        if ((strcmp(payload[2], "set") == 0) && _bc_talk_token_cmp(line, &tokens[3], "state") )
        {
            event.operation = BC_TALK_OPERATION_LED_SET;
            printf("aaa");
        }
        else if ((strcmp(payload[2], "get") == 0) )
        {
            event.operation = BC_TALK_OPERATION_LED_GET;
            callback(&event);
        }
    }

    //                    temp_length = tokens[i+1].end-tokens[i+1].start;
//                    strncpy(temp, line+tokens[i+1].start, temp_length );
//                    temp[temp_length] = 0;
//                    printf("%s \n", temp);


//    if (strncmp(line + tokens[1].start, "$config/sensors/", 16) == 0)
//    {
//        char tmp[100];
//        memset(tmp, 0, sizeof(tmp));
//        strncpy(tmp, line+tokens[1].start, tokens[1].end-tokens[1].start );
//        bc_log_debug("bc_talk_parse: payload: %s", tmp );
//    }

//    if (_bc_talk_jsoneq(line, &tokens[1], "$config/sensors/thermometer/i2c0-48/update") && (r==5))
//    {
//        if (_bc_talk_jsoneq(line, &tokens[3], "publish-interval"))
//        {
//            int number = strtol(line+tokens[4].start, NULL, 10);
//            printf("number %d \n", number);
//            bc_log_info("application_loop: thermometer new publish-interval %d", number);
////            if(thermometer_0_48) //TODO v tuto chvily by nemel byt problem, bud je inicializovany nebo neni
////            {
////                task_thermometer_set_interval(thermometer_0_48, (bc_tick_t)number);
////            }
//        }
//    }
//    else if(_bc_talk_jsoneq(line, &tokens[1], "relay/i2c0-3b/set") && (r==5))
//    {
////        if (relay && _bc_talk_jsoneq(line, &tokens[3], "state"))
////        {
////            if ( strncmp(line + tokens[4].start, "true", tokens[4].end - tokens[4].start) == 0)
////            {
////                task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NO);
////            }
////            else if ( strncmp(line + tokens[4].start, "false", tokens[4].end - tokens[4].start) == 0)
////            {
////                task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NC);
////            }
//
////        }
//    }

    free(payload_string);
}

static bool _bc_talk_token_cmp(char *line, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(line + tok->start, s, tok->end - tok->start) == 0)
    {
        return true;
    }
    return false;
}


static int _bc_talk_token_get_int(char *line, jsmntok_t *tok)
{
    char temp[10];
    memset(temp, 0x00, sizeof(temp));
    strncpy(&temp, line+tok->start, tok->end - tok->start < sizeof(temp) ? tok->end - tok->start : sizeof(temp) - 1 );
    return (int) strtol(temp, NULL, 10);
}

static bool _bc_talk_set_i2c(char *str, bc_talk_event_t *event)
{
    char * pEnd;
    int i2c_channel;
    if (strlen(str) != 7)
    {
        return false;
    }
    event->i2c_channel = (uint8_t)strtol(str+3, &pEnd, 10);
    event->device_address = (uint8_t)strtol(str+5, &pEnd, 16);

    return true;
}
