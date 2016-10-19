#include "task_display_oled.h"
#include "bc_log.h"
#include "bc_bridge.h"
#include "task.h"

void *task_display_oled(void *task_parameter)
{
    task_worker_t *self = (task_worker_t *) task_parameter;
    task_display_oled_parameters_t *parameters = (task_display_oled_parameters_t *)self->parameters;

    bc_i2c_ssd1306_t disp;
    bc_gfx_t gfx;

    bc_log_info("task_display_oled: started instance ");

    bc_i2c_interface_t interface;
    interface.bridge = self->_bridge;
    interface.channel = self->_i2c_channel;

    if (!bc_ic2_ssd1306_init(&disp, &interface, self->_device_address ))
    {
        bc_log_debug("task_display_oled: bc_ic2_ssd1306_init false bus %d, address 0x%02X",
                     (uint8_t) self->_i2c_channel, self->_device_address);

        return NULL;
    }

    task_worker_set_init_done(self);

    task_display_oled_parameters_t actual_parameters;

    bc_gfx_init(&gfx, disp.width, disp.height, disp.buffer);
    bc_gfx_clean(&gfx);

    int i;

    bc_os_mutex_lock(self->mutex);
    for (i=0; i<8; i++)
    {
        bc_gfx_clean_line(&gfx, i);
        bc_gfx_text(&gfx, parameters->lines[i] );
        strcpy(actual_parameters.lines[i], parameters->lines[i] );
    }
    bc_os_mutex_unlock(self->mutex);

    if (!bc_ic2_ssd1306_display(&disp))
    {
        return NULL;
    }

    while (true)
    {
        bc_os_semaphore_get(&self->semaphore);

        bc_os_mutex_lock(self->mutex);
        for (i=0; i<8; i++)
        {
            if (strcmp(actual_parameters.lines[i], parameters->lines[i] ) != 0 )
            {
                strcpy(actual_parameters.lines[i], parameters->lines[i] );
                bc_os_mutex_unlock(self->mutex);

                bc_gfx_clean_line(&gfx, i);
                bc_gfx_text(&gfx, actual_parameters.lines[i] );
                if (!bc_ic2_ssd1306_display_page(&disp, i))
                {
                    return NULL;
                }

                bc_os_mutex_lock(self->mutex);
            }

        }
        bc_os_mutex_unlock(self->mutex);

    }

    return NULL;
}

void task_display_oled_set_line(task_info_t *task_info, uint8_t line, char *text)
{
    if ( (line<0) || (line>7) )
    {
        bc_log_error("task_display_oled_set_line bad line index %d", line);
        return;
    }

    task_lock(task_info);
    strncpy( ((task_display_oled_parameters_t *)task_info->parameters)->lines[line], text, 21);
    task_unlock(task_info);

    task_semaphore_put(task_info);
}