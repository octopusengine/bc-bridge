#include "bc_talk.h"
#include "bc_os.h"
#include <jsmn.h>
#include "bc_log.h"

static bc_os_mutex_t bc_talk_mutex;
static bool _bc_talk_jsoneq(const char *json, jsmntok_t *tok, const char *s);

void bc_talk_init(void)
{
    bc_os_mutex_init(&bc_talk_mutex);
}

void bc_talk_publish_begin(char *topic)
{
    bc_os_mutex_lock(&bc_talk_mutex);

    fprintf(stdout, "[\"%s\", {", topic);
}

void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...)
{
    va_list ap;

    fprintf(stdout, "\"%s\": [", name);

    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    fprintf(stdout, ", \"%s\"], ", unit);
}

void bc_talk_publish_add_quantity_final(char *name, char *unit, char *value, ...)
{
    va_list ap;

    fprintf(stdout, "\"%s\": [", name);

    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    fprintf(stdout, ", \"%s\"]", unit);
}

void bc_talk_publish_end(void)
{
    fprintf(stdout, "}]\n");

    bc_os_mutex_unlock(&bc_talk_mutex);
}


bool bc_talk_parse(char *line, size_t length)
{
    jsmn_parser parser;
    jsmntok_t tokens[20];
    int r;

    jsmn_init(&parser);
    r = jsmn_parse(&parser, line, length, tokens, sizeof(tokens) );
    if (r < 0) {
        bc_log_error("application_loop: talk parser: Failed to parse JSON: %d", r);
        return false;
    }

    if (r < 1 || tokens[0].type != JSMN_ARRAY) {
        bc_log_error("application_loop: talk parser: Array expected");
        return false;
    }

    if (_bc_talk_jsoneq(line, &tokens[1], "$config/sensors/thermometer/i2c0-48/update") && (r==5))
    {
        if (_bc_talk_jsoneq(line, &tokens[3], "publish-interval"))
        {
            int number = strtol(line+tokens[4].start, NULL, 10);
            printf("number %d \n", number);
            bc_log_info("application_loop: thermometer new publish-interval %d", number);
//            if(thermometer_0_48) //TODO v tuto chvily by nemel byt problem, bud je inicializovany nebo neni
//            {
//                task_thermometer_set_interval(thermometer_0_48, (bc_tick_t)number);
//            }
        }
    }
    else if(_bc_talk_jsoneq(line, &tokens[1], "relay/i2c0-3b/set") && (r==5))
    {
//        if (relay && _bc_talk_jsoneq(line, &tokens[3], "state"))
//        {
//            if ( strncmp(line + tokens[4].start, "true", tokens[4].end - tokens[4].start) == 0)
//            {
//                task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NO);
//            }
//            else if ( strncmp(line + tokens[4].start, "false", tokens[4].end - tokens[4].start) == 0)
//            {
//                task_relay_set_mode(relay, BC_MODULE_RELAY_MODE_NC);
//            }

//        }
    }

}

static bool _bc_talk_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0)
    {
        return true;
    }
    return false;
}
