#include "timers.h"

#include "board_hal.h"
#include "clock.h"

enum
{
  TIMER_INPUT_CLOCK_HZ = F_SYS_CLOCK,
  TIMER_MILLISECOND_TICK_HZ = 1000,
  RED_STATUS_LED_PIN_MASK = BIT_08,
};

void status_timer7_init(uint32_t interval_milliseconds, bool enable_interrupt)
{
  // TIM7 is clocked from the 64 MHz peripheral clock. A 64,000 prescaler
  // divides that clock to 1 kHz, so each auto-reload tick is one millisecond.
  const uint32_t prescaler = TIMER_INPUT_CLOCK_HZ / TIMER_MILLISECOND_TICK_HZ;

  RCC->APBENR1 |= RCC_APBENR1_TIM7EN;
  TIM7->PSC = prescaler - 1u;
  TIM7->ARR = interval_milliseconds - 1u;
  TIM7->CNT = 0u;

  // Force the prescaler and auto-reload values into the timer before clearing
  // the update flag produced by this software-generated update event.
  TIM7->EGR = TIM_EGR_UG;
  TIM7->SR &= ~TIM_SR_UIF;

  if (enable_interrupt)
  {
    TIM7->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM7_LPTIM2_IRQn);
  }

  TIM7->CR1 = TIM_CR1_CEN;
}

void TIM7_IRQHandler(void)
{
  if ((TIM7->SR & TIM_SR_UIF) != 0u)
  {
    TIM7->SR &= ~TIM_SR_UIF;
    GPIOA->ODR ^= RED_STATUS_LED_PIN_MASK;
  }
}
