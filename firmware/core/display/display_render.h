#ifndef DISPLAY_RENDER_H
#define DISPLAY_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "display_surface.h"

typedef struct
{
  uint8_t color_bar_rotation;
  bool touch_button_pressed;
} display_render_context_t;

void display_render_draw_boot_screen(display_render_context_t *render_context,
                                     const display_surface_t *surface);
void display_render_rotate_color_bars(display_render_context_t *render_context,
                                      const display_surface_t *surface, int8_t delta);
void display_render_draw_touch_button(display_render_context_t *render_context,
                                      const display_surface_t *surface, bool pressed);

#endif // DISPLAY_RENDER_H
