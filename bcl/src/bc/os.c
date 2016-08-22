#include <bc/os.h>
#include <unistd.h>

void bc_os_sleep(bc_tick_t timeout)
{
    usleep(timeout*1000);
}
