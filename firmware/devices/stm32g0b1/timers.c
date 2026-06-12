#include "stm32g0b1_timers.h"

#include "clock.h"

void stm32g0b1_timer_initialize(TIM_TypeDef *timer,
                                uint32_t prescaler_count,
                                uint32_t period_ticks,
                                bool enable_update_interrupt,
                                IRQn_Type interrupt_number,
                                volatile uint32_t *clock_enable_register,
                                uint32_t clock_enable_mask)
{
  // Enable the selected timer through the caller-supplied APB clock register.
  // STM32G0B1 timers are split across APBENR1 and APBENR2, so the device helper
  // accepts the register and mask instead of embedding a board timer map.
  *clock_enable_register |= clock_enable_mask;

  timer->PSC = prescaler_count - 1u;
  timer->ARR = period_ticks - 1u;
  timer->CNT = 0u;

  // A software update transfers PSC and ARR into the active timer registers.
  // Clear the update flag it creates before interrupt delivery is enabled.
  timer->EGR = TIM_EGR_UG;
  timer->SR &= ~TIM_SR_UIF;
  timer->CR1 &= ~TIM_CR1_OPM;

  if (enable_update_interrupt)
  {
    timer->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(interrupt_number);
  }
  else
  {
    timer->DIER &= ~TIM_DIER_UIE;
  }

  timer->CR1 |= TIM_CR1_CEN;
}

void stm32g0b1_timer_set_period(TIM_TypeDef *timer, uint32_t period_ticks)
{
  const bool update_interrupt_was_enabled = (timer->DIER & TIM_DIER_UIE) != 0u;

  timer->DIER &= ~TIM_DIER_UIE;
  timer->ARR = period_ticks <= 1u ? 0u : period_ticks - 1u;
  timer->EGR = TIM_EGR_UG;
  timer->SR &= ~TIM_SR_UIF;

  if (update_interrupt_was_enabled)
  {
    timer->DIER |= TIM_DIER_UIE;
  }
}

void stm32g0b1_delay_microseconds(TIM_TypeDef *microsecond_timer, uint32_t microseconds)
{
  const uint32_t start_tick = microsecond_timer->CNT;
  while ((uint32_t)(microsecond_timer->CNT - start_tick) < microseconds)
  {
  }
}

void stm32g0b1_delay_milliseconds(uint32_t milliseconds)
{
  const uint32_t start_millisecond = get_sys_tick();
  while ((uint32_t)(get_sys_tick() - start_millisecond) < milliseconds)
  {
  }
}
