#ifndef STM32G0B1_TIMERS_H
#define STM32G0B1_TIMERS_H

#include <stdbool.h>
#include <stdint.h>

#include "board_hal.h"

// Configure one STM32G0B1 general-purpose or basic timer. The caller owns the
// timer choice, interrupt handler, clock-enable register, and clock-enable bit.
// Prescaler and period values are expressed as counts, not register encodings;
// this function performs the required minus-one conversion for PSC and ARR.
void stm32g0b1_timer_initialize(TIM_TypeDef *timer,
                                uint32_t prescaler_count,
                                uint32_t period_ticks,
                                bool enable_update_interrupt,
                                IRQn_Type interrupt_number,
                                volatile uint32_t *clock_enable_register,
                                uint32_t clock_enable_mask);

// Change a running timer period. The period is expressed in timer ticks and is
// loaded immediately with a software update event.
void stm32g0b1_timer_set_period(TIM_TypeDef *timer, uint32_t period_ticks);

// Busy-wait helpers for reset-time board setup. The microsecond helper requires
// a free-running timer configured at one tick per microsecond. Do not call these
// methods from scheduler tasks or interrupt handlers.
void stm32g0b1_delay_microseconds(TIM_TypeDef *microsecond_timer, uint32_t microseconds);
void stm32g0b1_delay_milliseconds(uint32_t milliseconds);

#endif // STM32G0B1_TIMERS_H
