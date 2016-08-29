#include "tags.h"

void tags_humidity_init(bc_tag_humidity_t *tag_humidity, bc_tag_interface_t *interface)
{
    bc_tag_humidity_init(tag_humidity, interface);
}

void tags_humidity_task(bc_tag_humidity_t *tag_humidity, tags_data_t *data)
{
    bc_tag_humidity_state_t state;

    data->null = true;

    if (!bc_tag_humidity_get_state(tag_humidity, &state))
    {
        return;
    }

    switch (state)
    {
        case BC_TAG_HUMIDITY_STATE_CALIBRATION_NOT_READ:
        {

            if (!bc_tag_humidity_read_calibration(tag_humidity))
            {
                return;
            }

            break;
        }
        case BC_TAG_HUMIDITY_STATE_POWER_DOWN:
        {
            if (!bc_tag_humidity_power_up(tag_humidity))
            {
                return;
            }

            break;
        }
        case BC_TAG_HUMIDITY_STATE_POWER_UP:
        {
            if (!bc_tag_humidity_one_shot_conversion(tag_humidity))
            {

                return;
            }

            break;
        }
        case BC_TAG_HUMIDITY_STATE_CONVERSION:
        {
            break;
        }
        case BC_TAG_HUMIDITY_STATE_RESULT_READY:
        {

            if (!bc_tag_humidity_read_result(tag_humidity))
            {
                return;
            }

            if (!bc_tag_humidity_get_result(tag_humidity, &data->value))
            {

                return;
            }

            if (!bc_tag_humidity_one_shot_conversion(tag_humidity))
            {
                return;
            }

            data->null = false;

            break;
        }
        default:
        {

            return;
        }
    }
}

void tags_temperature_init(bc_tag_temperature_t *tag_temperature, bc_tag_interface_t *interface,  uint8_t device_address)
{
    bc_tag_temperature_init(tag_temperature, interface, device_address);

    bc_tag_temperature_single_shot_conversion(tag_temperature);
}

void tags_temperature_task(bc_tag_temperature_t *tag_temperature, tags_data_t *data)
{
    bc_tag_temperature_state_t state;

    data->null = true;

    if (!bc_tag_temperature_get_state(tag_temperature, &state))
    {
        return;
    }

    switch (state)
    {
        case BC_TAG_TEMPERATURE_STATE_POWER_DOWN:
        {
            if (!bc_tag_temperature_read_temperature(tag_temperature))
            {
                return;
            }

            if (!bc_tag_temperature_get_temperature_celsius(tag_temperature, &data->value))
            {
                return;
            }

            if (!bc_tag_temperature_single_shot_conversion(tag_temperature))
            {
                return;
            }

            data->null = false;

            break;
        }
        case BC_TAG_TEMPERATURE_STATE_CONVERSION:
        {
            bc_tag_temperature_power_down(tag_temperature);

            break;
        }
        default:
        {
            bc_tag_temperature_power_down(tag_temperature);

            break;
        }
    }
}


void tags_lux_meter_init(bc_tag_lux_meter_t *tag_lux_meter, bc_tag_interface_t *interface)
{
    bc_tag_lux_meter_init(tag_lux_meter, interface, BC_TAG_LUX_METER_ADDRESS_DEFAULT);
}

void tags_lux_meter_task(bc_tag_lux_meter_t *tag_lux_meter, tags_data_t *data)
{
    bc_tag_lux_meter_state_t state;

    data->null = true;

    if (!bc_tag_lux_meter_get_state(tag_lux_meter, &state))
    {
        return;
    }

    switch (state)
    {
        case BC_TAG_LUX_METER_STATE_POWER_DOWN:
        {
            if (!bc_tag_lux_meter_single_shot_conversion(tag_lux_meter))
            {
                return;
            }

            break;
        }
        case BC_TAG_LUX_METER_STATE_CONVERSION:
        {
            break;
        }
        case BC_TAG_LUX_METER_STATE_RESULT_READY:
        {
            if (!bc_tag_lux_meter_read_result(tag_lux_meter))
            {
                return;
            }

            if (!bc_tag_lux_meter_get_result_lux(tag_lux_meter, &data->value))
            {
                return;
            }

            if (!bc_tag_lux_meter_single_shot_conversion(tag_lux_meter))
            {
                return;
            }

            data->null = false;

            break;
        }
        default:
        {
            break;
        }
    }
}

void tags_barometer_init(bc_tag_barometer_t *tag_barometer, bc_tag_interface_t *interface)
{
    bc_tag_barometer_init(tag_barometer, interface);
}

void tags_barometer_task(bc_tag_barometer_t *tag_barometer, tags_data_t *absolute_pressure, tags_data_t *altitude)
{
    bc_tag_barometer_state_t state;

    absolute_pressure->null = true;
    altitude->null = true;

    if (!bc_tag_barometer_get_state(tag_barometer, &state))
    {
        return;
    }

    switch (state)
    {
        case BC_TAG_BAROMETER_STATE_POWER_DOWN:
        {
            if (!bc_tag_barometer_one_shot_conversion_altitude(tag_barometer))
            {
                return;
            }

            break;
        }
        case BC_TAG_BAROMETER_STATE_RESULT_READY_ALTITUDE:
        {
            if (!bc_tag_barometer_read_result(tag_barometer))
            {
                return;
            }

            if (!bc_tag_barometer_get_altitude(tag_barometer, &altitude->value))
            {
                return;
            }

            if (!bc_tag_barometer_one_shot_conversion_pressure(tag_barometer))
            {
                return;
            }

            altitude->value /= 100.f;//TODO haze to divne veci pridal jsem deleni
            altitude->null = false;

            break;
        }
        case BC_TAG_BAROMETER_STATE_RESULT_READY_PRESSURE:
        {
            if (!bc_tag_barometer_read_result(tag_barometer))
            {
                return;
            }

            if (!bc_tag_barometer_get_pressure(tag_barometer, &absolute_pressure->value))
            {
                return;
            }

            absolute_pressure->value /= 1000.f;

            if (!bc_tag_barometer_one_shot_conversion_altitude(tag_barometer))
            {
                return;
            }

            absolute_pressure->null = false;

            break;
        }
        case BC_TAG_BAROMETER_STATE_CONVERSION:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}
