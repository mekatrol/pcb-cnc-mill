#include "timers.h"

#include "board_hal.h"
#include "stm32g0b1_timers.h"

enum
{
  RED_STATUS_LED_PIN_MASK = BIT_08,
};

void status_timer7_init(uint32_t interval_milliseconds, bool enable_interrupt)
{
  // The shared STM32G0B1 timer helper owns register setup. This board wrapper
  // selects TIM7 and a one-millisecond timer tick for the PA8 status LED.
  stm32g0b1_timer_initialize(TIM7,
                             64000u,
                             interval_milliseconds,
                             enable_interrupt,
                             TIM7_LPTIM2_IRQn,
                             &RCC->APBENR1,
                             RCC_APBENR1_TIM7EN);
}

void TIM7_IRQHandler(void)
{
  if ((TIM7->SR & TIM_SR_UIF) != 0u)
  {
    TIM7->SR &= ~TIM_SR_UIF;
    GPIOA->ODR ^= RED_STATUS_LED_PIN_MASK;
  }
}
