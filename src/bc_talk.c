#include "bc_talk.h"
#include "bc_os.h"

static bc_os_mutex_t bc_talk_mutex;

void bc_talk_init(void)
{
    bc_os_mutex_init(&bc_talk_mutex);
}

void bc_talk_publish_begin(const char *topic)
{
    bc_os_mutex_lock(&bc_talk_mutex);

    printf("[\"%s\", {", topic);
}

void bc_talk_publish_add_quantity(const char *name, const char *unit, const char *value, ...)
{
    va_list ap;

    printf("\"%s\": [", name);

    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    printf(", \"%s\"], ", unit);
}

void bc_talk_publish_add_quantity_final(const char *name, const char *unit, const char *value, ...)
{
    va_list ap;

    printf("\"%s\": [", name);

    va_start(ap, value);
    vfprintf(stdout, value, ap);
    va_end(ap);

    printf(", \"%s\"]", unit);
}

void bc_talk_publish_end(void)
{
    printf("}]\n");

    bc_os_mutex_unlock(&bc_talk_mutex);
}
