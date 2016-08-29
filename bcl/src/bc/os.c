#include <bc/os.h>
#include <unistd.h>

void bc_os_sleep(bc_tick_t milisec)
{
    usleep(milisec*1000);
}

void bc_os_mutex_init(bc_os_mutex_t *mutex)
{
    pthread_mutex_init(mutex,NULL);
}

void bc_os_mutex_lock(bc_os_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
}

void bc_os_mutex_unlock(bc_os_mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
}