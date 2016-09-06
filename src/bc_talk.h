#ifndef _BC_TALK_H
#define _BC_TALK_H

#include "bc_common.h"

void bc_talk_init(void);
void bc_talk_publish_begin(char *topic);
void bc_talk_publish_add_quantity(char *name, char *unit, char *value, ...);
void bc_talk_publish_add_quantity_final(char *name, char *unit, char *value, ...);
void bc_talk_publish_end(void);

#endif /* _BC_TALK_H */
