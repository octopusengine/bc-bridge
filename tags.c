#include "tags.h"



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
