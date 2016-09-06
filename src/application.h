#ifndef _APPLICATION_H
#define _APPLICATION_H

#include "bc_common.h"
#include "bc_log.h"

void application_init(bool wait_start_string, bc_log_level_t log_level);
void application_loop(bool *quit);

#endif /* _APPLICATION_H */
