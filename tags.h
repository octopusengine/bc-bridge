#ifndef BC_BRIDGE_TAGS_H
#define BC_BRIDGE_TAGS_H

#ifdef BRIDGE
#include <bc/bridge.h>
#endif

#include <bc/tag.h>
#include <bc/tag/humidity.h>
#include <bc/tag/temperature.h>
#include <bc/module/relay.h>

typedef struct {
    bool null;
    float value;
    void *tag;
} tags_data_t;

void tags_humidity_task(bc_tag_humidity_t *tag_humidity, tags_data_t *data);

#endif //BC_BRIDGE_TAGS_H
