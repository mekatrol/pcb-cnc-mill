#include "display_render.h"

#include "display_strings.h"

static void draw_large_tft_color_bars(display_render_context_t *render_context,
                                      const display_surface_t *surface)
{
  const uint16_t colors[] = {
      display_rgb565(210, 48, 48),
      display_rgb565(48, 180, 90),
      display_rgb565(48, 96, 220),
      display_rgb565(255, 190, 48),
      display_rgb565(180, 72, 220),
      display_rgb565(40, 210, 210),
  };

  const uint16_t segment_width = (uint16_t)(surface->width_pixels / 6u);
  for (uint8_t segment = 0; segment < 6; segment++)
  {
    const uint8_t color_index = (uint8_t)((segment + render_context->color_bar_rotation) % 6u);
    surface->fill_rect(surface->context, (uint16_t)(segment * segment_width), 0, segment_width,
                       24, colors[color_index]);
  }
}

void display_render_draw_touch_button(display_render_context_t *render_context,
                                      const display_surface_t *surface, bool pressed)
{
  const uint16_t fill = pressed ? display_rgb565(64, 210, 230) : display_rgb565(255, 190, 48);
  const uint16_t panel =
      pressed ? display_rgb565(96, 235, 255) : display_rgb565(255, 205, 82);
  const uint16_t text = display_rgb565(12, 18, 28);

  render_context->touch_button_pressed = pressed;
  surface->fill_rect(surface->context, 98, 214, 284, 58, fill);
  surface->fill_rect(surface->context, 102, 218, 276, 50, panel);
  display_surface_draw_text(surface, 150, 232, display_string_touch_chirp_action, text, panel, 2);
}

void display_render_draw_boot_screen(display_render_context_t *render_context,
                                     const display_surface_t *surface)
{
  const uint16_t background = display_rgb565(12, 18, 28);
  const uint16_t white = display_rgb565(255, 255, 255);
  const uint16_t amber = display_rgb565(255, 190, 48);
  const uint16_t cyan = display_rgb565(64, 210, 230);

  render_context->color_bar_rotation = 0;
  surface->fill(surface->context, background);
  draw_large_tft_color_bars(render_context, surface);
  surface->fill_rect(surface->context, 0, 296, surface->width_pixels, 24, amber);

  display_surface_draw_text(surface, 54, 26, display_string_brand_name, white, background, 4);
  display_surface_draw_text(surface, 54, 70, display_string_product_name, white, background, 4);
  display_surface_draw_text(surface, 84, 128, display_string_large_tft_profile_name, cyan,
                            background, 3);
  display_surface_draw_text(surface, 72, 174, display_string_encoder_rgb_hint, amber, background,
                            2);
  display_render_draw_touch_button(render_context, surface, false);
  display_surface_draw_text(surface, 92, 300, display_string_encoder_chirp_hint, background, amber,
                            2);
}

void display_render_rotate_color_bars(display_render_context_t *render_context,
                                      const display_surface_t *surface, int8_t delta)
{
  if (delta > 0)
  {
    render_context->color_bar_rotation = (uint8_t)((render_context->color_bar_rotation + 1u) % 6u);
  }
  else if (delta < 0)
  {
    render_context->color_bar_rotation = (uint8_t)((render_context->color_bar_rotation + 5u) % 6u);
  }
  else
  {
    return;
  }

  draw_large_tft_color_bars(render_context, surface);
}
