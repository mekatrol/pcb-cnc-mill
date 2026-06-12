#include "timers.h"

#include "board_hal.h"
#include "stm32g0b1_timers.h"

void stepper_interrupt(void);

void delay_us(uint32_t microseconds)
{
  // TIM6 is configured as a free-running one-megahertz counter during board
  // initialization, so each counter increment represents one microsecond.
  stm32g0b1_delay_microseconds(TIM6, microseconds);
}

void delay_ms(uint32_t milliseconds)
{
  stm32g0b1_delay_milliseconds(milliseconds);
}

void timer6_init(void)
{
  // 64 MHz / 64 = 1 MHz. A 65,536-tick period uses the full 16-bit counter
  // range and gives delay_us wrap-safe elapsed-time subtraction.
  stm32g0b1_timer_initialize(TIM6,
                             64u,
                             65536u,
                             false,
                             TIM6_DAC_LPTIM1_IRQn,
                             &RCC->APBENR1,
                             RCC_APBENR1_TIM6EN);
}

void status_timer7_init(uint32_t interval_milliseconds, bool enable_interrupt)
{
  // 64 MHz / 64,000 = 1 kHz, so one TIM7 tick is one millisecond.
  stm32g0b1_timer_initialize(TIM7,
                             64000u,
                             interval_milliseconds,
                             enable_interrupt,
                             TIM7_LPTIM2_IRQn,
                             &RCC->APBENR1,
                             RCC_APBENR1_TIM7EN);
}

void stepper_timer14_init(void)
{
  // 64 MHz / 64 = 1 MHz. Ten timer ticks produce a 10 microsecond stepper
  // service interval and preserve the existing minimum pulse width.
  stm32g0b1_timer_initialize(TIM14,
                             64u,
                             10u,
                             true,
                             TIM14_IRQn,
                             &RCC->APBENR2,
                             RCC_APBENR2_TIM14EN);
}

void set_timer7_interval(uint32_t interval_milliseconds)
{
  stm32g0b1_timer_set_period(TIM7, interval_milliseconds);
}

void TIM6_DAC_IRQHandler(void)
{
  if ((TIM6->SR & TIM_SR_UIF) != 0u)
  {
    TIM6->SR &= ~TIM_SR_UIF;
  }
}

void TIM7_IRQHandler(void)
{
  if ((TIM7->SR & TIM_SR_UIF) != 0u)
  {
    TIM7->SR &= ~TIM_SR_UIF;
    GPIOD->ODR ^= BIT_08;
  }
}

void TIM14_IRQHandler(void)
{
  if ((TIM14->SR & TIM_SR_UIF) != 0u)
  {
    TIM14->SR &= ~TIM_SR_UIF;
    stepper_interrupt();
  }
}
