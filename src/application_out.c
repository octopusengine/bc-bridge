#include "application_out.h"
#include "bc_common.h"
#include "bc_os.h"

static bc_os_mutex_t application_out_mutex;
static bool application_out_is_init = false;

void application_out_init(void)
{
    if (application_out_is_init)
    {
        return;
    }
    bc_os_mutex_init(&application_out_mutex);
    application_out_is_init = true;
}

void application_out_write(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&application_out_mutex);

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);

    fprintf(stdout, "\n");
    fflush(stdout);

    bc_os_mutex_unlock(&application_out_mutex);
}
