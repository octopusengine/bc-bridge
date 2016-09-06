#ifndef _BC_TALK_H
#define _BC_TALK_H

#include "bc_common.h"

void bc_talk_init(void);
void bc_talk_publish_begin(const char *topic);
void bc_talk_publish_add_quantity(const char *name, const char *unit, const char *value, ...);
void bc_talk_publish_add_quantity_final(const char *name, const char *unit, const char *value, ...);
void bc_talk_publish_end(void);

#endif /* _BC_TALK_H */
