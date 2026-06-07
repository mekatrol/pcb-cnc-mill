#include <stdint.h>

static volatile uint32_t idle_counter;

void mainboard_initialize_hardware(void)
{
  // Placeholder until STM32G0 clock, GPIO, timer, USB, and safety inputs are
  // brought up for the SKR Mini E3 V3 main controller board.
}

void mainboard_run_background_tasks(void)
{
  idle_counter++;
}
