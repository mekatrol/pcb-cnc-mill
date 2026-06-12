#include "toolhead_hal.h"

enum
{
  RED_STATUS_LED_PIN_POSITION = 8,
  RED_STATUS_LED_TOGGLE_INTERVAL_MS = 1000,
};

static void initialize_status_led_gpio(void)
{
  // The EBB42 Gen2 schematic labels the onboard red LED signal RLED and
  // connects it to STM32G0B1 pin PA8. Configure only that pin for this first
  // bring-up image so unrelated toolhead outputs remain in reset state.
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
  GPIO_SET_MODE(GPIOA, RED_STATUS_LED_PIN_POSITION, MODER_OUT);

  // RLED drives the indicator through its series resistor to ground, so a low
  // output starts with the LED off before the periodic timer is enabled.
  GPIOA->BRR = BIT_08;
}

void toolhead_initialize_hardware(void)
{
  // The EBB42 Gen2 and SKR Mini E3 V3 both use an STM32G0B1 with an 8 MHz
  // external crystal. Reuse the proven 64 MHz clock setup from the SKR board.
  clock_init();
  initialize_status_led_gpio();

  // TIM7 generates one bounded interrupt each second. The handler only clears
  // the update flag and toggles PA8; all later toolhead service work should use
  // the shared scheduler rather than growing this interrupt handler.
  status_timer7_init(RED_STATUS_LED_TOGGLE_INTERVAL_MS, true);
  interrupts_enable();
}

void toolhead_wait_for_interrupt(void)
{
  __WFI();
}
