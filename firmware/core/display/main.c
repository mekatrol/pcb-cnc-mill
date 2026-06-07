#include "display_hal.h"

int main(void)
{
  // Keep the shared display entry point hardware-neutral. The selected display
  // board owns clocks, pins, LCD bus setup, touch inputs, and local UI polling.
  display_initialize_hardware();

  while (1)
  {
    display_run_background_tasks();
  }
}
