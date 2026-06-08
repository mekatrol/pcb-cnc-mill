#include "display_render.h"

#include "display_strings.h"

enum
{
  LARGE_TFT_WIDTH_PIXELS = 480,
  LARGE_TFT_STATUS_HEIGHT_PIXELS = 64,
  LARGE_TFT_ACTION_ROW_TOP_PIXELS = 224,
  LARGE_TFT_ACTION_ROW_HEIGHT_PIXELS = 48,
  LARGE_TFT_ACTION_COLUMN_WIDTH_PIXELS = 120,
};

static void draw_horizontal_line(const display_surface_t *surface, uint16_t y, uint16_t color)
{
  surface->fill_rect(surface->context, 0, y, surface->width_pixels, 2, color);
}

static void draw_vertical_line(const display_surface_t *surface, uint16_t x, uint16_t y,
                               uint16_t height, uint16_t color)
{
  surface->fill_rect(surface->context, x, y, 2, height, color);
}

static void draw_button_cell(const display_surface_t *surface, uint16_t column, uint16_t row,
                             const char *label, uint16_t text_color, uint16_t background_color,
                             uint16_t border_color)
{
  const uint16_t x = (uint16_t)(column * LARGE_TFT_ACTION_COLUMN_WIDTH_PIXELS);
  const uint16_t y =
      (uint16_t)(LARGE_TFT_ACTION_ROW_TOP_PIXELS + row * LARGE_TFT_ACTION_ROW_HEIGHT_PIXELS);

  surface->fill_rect(surface->context, x, y, LARGE_TFT_ACTION_COLUMN_WIDTH_PIXELS,
                     LARGE_TFT_ACTION_ROW_HEIGHT_PIXELS, background_color);
  draw_vertical_line(surface, x, y, LARGE_TFT_ACTION_ROW_HEIGHT_PIXELS, border_color);
  draw_horizontal_line(surface, y, border_color);
  display_surface_draw_text(surface, (uint16_t)(x + 14u), (uint16_t)(y + 16u), label, text_color,
                            background_color, 2);
}

void display_render_draw_home_screen(const display_surface_t *surface)
{
  const uint16_t background = display_rgb565(10, 14, 18);
  const uint16_t panel = display_rgb565(22, 30, 38);
  const uint16_t panel_alt = display_rgb565(30, 38, 46);
  const uint16_t border = display_rgb565(96, 120, 132);
  const uint16_t text = display_rgb565(232, 238, 232);
  const uint16_t muted = display_rgb565(150, 164, 168);
  const uint16_t warning = display_rgb565(232, 170, 42);
  const uint16_t alarm = display_rgb565(210, 70, 64);
  const uint16_t action = display_rgb565(40, 56, 68);
  const uint16_t disabled_action = display_rgb565(34, 40, 46);

  surface->fill(surface->context, background);
  surface->fill_rect(surface->context, 0, 0, surface->width_pixels, LARGE_TFT_STATUS_HEIGHT_PIXELS,
                     panel);
  draw_horizontal_line(surface, 62, border);

  display_surface_draw_text(surface, 10, 12, display_string_product_name, text, panel, 2);
  display_surface_draw_text(surface, 228, 12, display_string_home_machine_state, warning, panel, 2);
  display_surface_draw_text(surface, 396, 12, display_string_home_link_state, alarm, panel, 2);
  display_surface_draw_text(surface, 10, 38, display_string_home_status_line, muted, panel, 1);

  display_surface_draw_text(surface, 18, 82, display_string_home_machine_position, text, background,
                            2);
  display_surface_draw_text(surface, 18, 118, display_string_home_work_position, text, background,
                            2);
  display_surface_draw_text(surface, 18, 164, display_string_home_job_status, muted, background, 1);
  display_surface_draw_text(surface, 18, 188, display_string_home_limits_status, muted, background,
                            1);

  draw_horizontal_line(surface, 222, border);
  draw_button_cell(surface, 0, 0, display_string_home_action_home, muted, disabled_action, border);
  draw_button_cell(surface, 1, 0, display_string_home_action_motion, muted, disabled_action, border);
  draw_button_cell(surface, 2, 0, display_string_home_action_spindle, muted, disabled_action,
                   border);
  draw_button_cell(surface, 3, 0, display_string_home_action_probe, muted, disabled_action, border);
  draw_button_cell(surface, 0, 1, display_string_home_action_job, muted, disabled_action, border);
  draw_button_cell(surface, 1, 1, display_string_home_action_coordinates, text, action, border);
  draw_button_cell(surface, 2, 1, display_string_home_action_settings, text, action, border);
  draw_button_cell(surface, 3, 1, display_string_home_action_safety, text, panel_alt, border);

  draw_vertical_line(surface, LARGE_TFT_WIDTH_PIXELS - 2u, LARGE_TFT_ACTION_ROW_TOP_PIXELS,
                     (uint16_t)(2u * LARGE_TFT_ACTION_ROW_HEIGHT_PIXELS), border);
  draw_horizontal_line(surface, 318, border);
}
