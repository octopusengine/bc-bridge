#ifndef _BC_LOG_H
#define _BC_LOG_H

#include "bc_common.h"

typedef enum
{
    BC_LOG_LEVEL_DUMP = 0,
    BC_LOG_LEVEL_DEBUG = 1,
    BC_LOG_LEVEL_INFO = 2,
    BC_LOG_LEVEL_WARNING = 3,
    BC_LOG_LEVEL_ERROR = 4,
    BC_LOG_LEVEL_FATAL = 5

} bc_log_level_t;

void bc_log_init(bc_log_level_t level);
void bc_log_dump(const void *buffer, uint32_t length, const char *format, ...);
void bc_log_debug(const char *format, ...);
void bc_log_info(const char *format, ...);
void bc_log_warning(const char *format, ...);
void bc_log_error(const char *format, ...);
void bc_log_fatal(const char *format, ...);

#endif /* _BC_LOG_H */
