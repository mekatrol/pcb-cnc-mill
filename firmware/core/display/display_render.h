#ifndef DISPLAY_RENDER_H
#define DISPLAY_RENDER_H

#include "display_surface.h"

typedef enum
{
  DISPLAY_LINK_STATE_WAITING,
  DISPLAY_LINK_STATE_OK,
  DISPLAY_LINK_STATE_LOST,
} display_link_state_t;

void display_render_draw_home_screen(const display_surface_t *surface, display_link_state_t link_state);
void display_render_draw_home_link_state(const display_surface_t *surface, display_link_state_t link_state);

#endif // DISPLAY_RENDER_H
