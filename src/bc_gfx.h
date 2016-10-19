#ifndef BC_GFX_H
#define BC_GFX_H

#include "bc_common.h"

typedef struct
{
    uint8_t _width;
    uint8_t _height;
    uint8_t *_buffer;
    size_t _length;
    uint8_t _pages;
    int _cursor;

} bc_gfx_t;

void bc_gfx_init(bc_gfx_t *self, uint8_t width, uint8_t height, uint8_t *buffer);
void bc_gfx_clean(bc_gfx_t *self);
void bc_gfx_text(bc_gfx_t *self, char *text);
void bc_gfx_newline(bc_gfx_t *self);
void bc_gfx_set_line(bc_gfx_t *self, uint8_t line);
void bc_gfx_clean_line(bc_gfx_t *self, uint8_t line);

#endif //BC_GFX_H
