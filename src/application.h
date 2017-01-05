#ifndef _APPLICATION_H
#define _APPLICATION_H

#include "bc_common.h"
#include "bc_log.h"

typedef struct
{
    bool furious_mode;
    bc_log_level_t log_level;
    bool dev_list;
    int dev_id;
    char *dev_path;

} application_parameters_t;

void application_init(application_parameters_t *parameters);
void application_loop(bool *quit, application_parameters_t *parameters);

#endif /* _APPLICATION_H */
