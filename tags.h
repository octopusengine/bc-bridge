#ifndef BC_BRIDGE_TAGS_H
#define BC_BRIDGE_TAGS_H

#ifdef BRIDGE
#include <bc/bridge.h>
#endif

#include <bc/tag.h>
#include <bc/tag/humidity.h>
#include <bc/tag/temperature.h>
#include <bc/tag/lux_meter.h>
#include <bc/tag/barometer.h>
#include <bc/module/relay.h>

typedef struct {
    bool null;
    float value;
    void *tag;
} tags_data_t;

void tags_humidity_init(bc_tag_humidity_t *tag_humidity, bc_tag_interface_t *interface);
void tags_humidity_task(bc_tag_humidity_t *tag_humidity, tags_data_t *data);
void tags_temperature_init(bc_tag_temperature_t *tag_temperature, bc_tag_interface_t *interface,  uint8_t device_address);
void tags_temperature_task(bc_tag_temperature_t *tag_temperature, tags_data_t *data);
void tags_lux_meter_init(bc_tag_lux_meter_t *tag_lux_meter, bc_tag_interface_t *interface);
void tags_lux_meter_task(bc_tag_lux_meter_t *tag_lux_meter, tags_data_t *data);
void tags_barometer_init(bc_tag_barometer_t *tag_barometer, bc_tag_interface_t *interface);
void tags_barometer_task(bc_tag_barometer_t *tag_barometer, tags_data_t *absolute_pressure, tags_data_t *altitude);

#endif //BC_BRIDGE_TAGS_H
