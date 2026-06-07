#include "mainboard_hal.h"

int main(void)
{
  // Keep the shared mainboard entry point hardware-neutral. Board-specific
  // startup, pin setup, clocks, and peripheral quirks live behind this HAL.
  mainboard_initialize_hardware();

  while (1)
  {
    mainboard_run_background_tasks();
  }
}
