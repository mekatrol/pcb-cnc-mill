#include "mainboard_hal.h"

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
#include "mainboard_display_module.h"
#endif

int main(void)
{
  // Keep the shared mainboard entry point hardware-neutral. Board-specific
  // startup, pin setup, clocks, and peripheral quirks live behind this HAL.
  mainboard_initialize_hardware();

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
  // A mainboard-attached display module shares this firmware image and the
  // mainboard clock tree. The module may initialize only panel-local hardware
  // such as display pins, encoder inputs, chip-select lines, sounders, or LEDs.
  mainboard_display_module_initialize_hardware();
#endif

  while (1)
  {
    mainboard_run_background_tasks();

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
    mainboard_display_module_run_background_tasks();
    mainboard_display_module_run_feedback_tasks();
#endif
  }
}
