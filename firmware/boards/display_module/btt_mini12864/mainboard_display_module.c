#include <stdint.h>

#include "mainboard_display_module.h"

static volatile uint32_t display_module_service_counter;
static volatile uint32_t feedback_service_counter;

void mainboard_display_module_initialize_hardware(void)
{
  // Placeholder for the BTT Mini12864 attached-panel bring-up. This module does
  // not own a reset handler, linker script, system clock, or scheduler. The
  // selected mainboard firmware image owns those resources and calls this
  // module after the mainboard HAL has made GPIO and timer setup safe.
}

void mainboard_display_module_run_background_tasks(void)
{
  // Future bounded work belongs here: sample the rotary encoder and click
  // button, scan any panel-local inputs, and advance small display redraw
  // regions for the 128x64 screen.
  display_module_service_counter++;
}

void mainboard_display_module_run_feedback_tasks(void)
{
  // Future bounded feedback work belongs here: service panel sounder chirps,
  // status LEDs, or backlight timeout state without blocking machine control.
  feedback_service_counter++;
}
