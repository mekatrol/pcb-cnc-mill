#ifndef DISPLAY_SURFACE_H
#define DISPLAY_SURFACE_H

#include <stdint.h>

typedef struct
{
  void *context;
  uint16_t width_pixels;
  uint16_t height_pixels;
  void (*fill)(void *context, uint16_t color);
  void (*fill_rect)(void *context, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint16_t color);
} display_surface_t;

uint16_t display_rgb565(uint8_t red, uint8_t green, uint8_t blue);
void display_surface_draw_text(const display_surface_t *surface, uint16_t x, uint16_t y,
                               const char *text, uint16_t foreground, uint16_t background,
                               uint8_t scale);

#endif // DISPLAY_SURFACE_H
