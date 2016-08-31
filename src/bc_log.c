#include "bc_log.h"
#include "bc_os.h"
#include <sys/time.h>
#include <time.h>

static bc_os_mutex_t bc_log_mutex;
static bc_log_level_t bc_log_level;

static void bc_log_head(bc_log_level_t level);
static void bc_log_message(const char *format, va_list ap);
static void bc_log_tail(void);

void bc_log_init(bc_log_level_t level)
{
    bc_os_mutex_init(&bc_log_mutex);

    bc_log_level = level;
}

void bc_log_dump(const void *buffer, uint32_t length, const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_DUMP)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_ERROR);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    // TODO Dump buffer

    bc_os_mutex_unlock(&bc_log_mutex);
}

void bc_log_debug(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_DEBUG)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_DEBUG);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    bc_os_mutex_unlock(&bc_log_mutex);
}

void bc_log_info(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_INFO)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_INFO);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    bc_os_mutex_unlock(&bc_log_mutex);
}

void bc_log_warning(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_WARNING)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_WARNING);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    bc_os_mutex_unlock(&bc_log_mutex);
}

void bc_log_error(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_ERROR)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_ERROR);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    bc_os_mutex_unlock(&bc_log_mutex);
}

void bc_log_fatal(const char *format, ...)
{
    va_list ap;

    bc_os_mutex_lock(&bc_log_mutex);

    if ((int32_t) bc_log_level > (int32_t) BC_LOG_LEVEL_FATAL)
    {
        bc_os_mutex_unlock(&bc_log_mutex);

        return;
    }

    bc_log_head(BC_LOG_LEVEL_FATAL);

    va_start(ap, format);
    bc_log_message(format, ap);
    va_end(ap);

    bc_log_tail();

    bc_os_mutex_unlock(&bc_log_mutex);
}

static void bc_log_head(bc_log_level_t level)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) == 0)
    {
        struct tm tm;

        if (localtime_r(&tv.tv_sec, &tm) != NULL)
        {
            char buffer[64];

            if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S.%%03d ", &tm) != 0)
            {
                fprintf(stderr, buffer, tv.tv_usec / 1000);
            }
        }
    }

    switch (level)
    {
        case BC_LOG_LEVEL_DUMP:
            fprintf(stderr, "[DUMP ] ");
            break;
        case BC_LOG_LEVEL_DEBUG:
            fprintf(stderr, "[DEBUG] ");
            break;
        case BC_LOG_LEVEL_INFO:
            fprintf(stderr, "[INFO ] ");
            break;
        case BC_LOG_LEVEL_WARNING:
            fprintf(stderr, "[WARN ] ");
            break;
        case BC_LOG_LEVEL_ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case BC_LOG_LEVEL_FATAL:
            fprintf(stderr, "[FATAL] ");
            break;
        default:
            fprintf(stderr, "[?????] ");
            break;
    }
}

static void bc_log_message(const char *format, va_list ap)
{
    vfprintf(stderr, format, ap);
}

static void bc_log_tail(void)
{
    fprintf(stderr, "\n");
    fflush(stderr);
}
