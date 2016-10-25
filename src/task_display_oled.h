#ifndef _TASK_DISPLAY_OLED_H
#define _TASK_DISPLAY_OLED_H

#include "bc_common.h"
#include "bc_os.h"
#include "task.h"
#include "bc_i2c_ssd1306.h"
#include "bc_gfx.h"

typedef struct
{
    char lines[8][22];
    uint8_t *raw;

} task_display_oled_parameters_t;

void *task_display_oled(void *task_parameter);
void task_display_oled_set_line(task_info_t *task_info, uint8_t line, char *text);
void task_display_oled_set_raw(task_info_t *task_info, uint8_t *buffer);

#endif /* _TASK_DISPLAY_OLED_H */
