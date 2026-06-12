#include "toolhead_hal.h"

int main(void)
{
  // Keep the shared toolhead entry point hardware-neutral. The selected board
  // owns its clock tree, pin map, timer choice, and interrupt handlers.
  toolhead_initialize_hardware();

  while (1)
  {
    // This first toolhead smoke test has no scheduled work. Sleep until TIM7
    // wakes the core to run the status LED interrupt handler.
    toolhead_wait_for_interrupt();
  }
}
