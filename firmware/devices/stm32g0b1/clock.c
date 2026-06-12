
#include <stdint.h>

#include "board_hal.h"
#include "clock.h"

volatile uint32_t systick_global = 0;
volatile uint32_t second_counter_global = 0;

#define HAL_TICK_FREQ_1KHZ 1U

static inline void nvic_set_priority(IRQn_Type IRQn, uint32_t priority)
{
  if ((int32_t)(IRQn) >= 0)
  {
    NVIC->IP[_IP_IDX(IRQn)] = ((uint32_t)(NVIC->IP[_IP_IDX(IRQn)] & ~(0xFFUL << _BIT_SHIFT(IRQn))) |
                               (((priority << (8U - __NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << _BIT_SHIFT(IRQn)));
  }
  else
  {
    SCB->SHP[_SHP_IDX(IRQn)] = ((uint32_t)(SCB->SHP[_SHP_IDX(IRQn)] & ~(0xFFUL << _BIT_SHIFT(IRQn))) |
                                (((priority << (8U - __NVIC_PRIO_BITS)) & (uint32_t)0xFFUL) << _BIT_SHIFT(IRQn)));
  }
}

static inline void sys_tick_init(uint32_t ticks)
{
  if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk)
  {
    return; // Invalid reload value
  }

  SysTick->LOAD = (uint32_t)(ticks - 1UL);                          /* set reload register */
  nvic_set_priority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
  SysTick->VAL = 0UL;                                               /* Load the SysTick Counter Value */
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk; /* Enable SysTick IRQ and SysTick Timer */
}

static void reset_system_clock_to_internal_high_speed_clock(void)
{
  // Start from a known-safe clock source before changing PLL fields. A
  // bootloader may jump here with SYSCLK still sourced from its PLL setup.
  RCC->CR |= RCC_CR_HSION;
  RCC->CR &= ~RCC_CR_HSIDIV;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0)
    ;

  RCC->CFGR &= ~RCC_CFGR_SW;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSISYS)
    ;

  RCC->CR &= ~RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) != 0)
    ;

  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE);
}

void clock_init(void)
{
  RCC->CR |= RCC_CR_HSEON; // Enable HSE
  while (!(RCC->CR & RCC_CR_HSERDY))
    ; // Wait for HSE ready
  reset_system_clock_to_internal_high_speed_clock();

  RCC->PLLCFGR =
      (0b11 << 0) |  // PLLSRC = HSE
      (0b000 << 4) | // PLLM = /1
      (16 << 8) |    // PLLN = 16
      (0b00 << 25) | // PLLR = /2
      (1 << 28);     // PLLREN = enable PLLR output

  RCC->CR |= RCC_CR_PLLON; // Enable PLL
  while (!(RCC->CR & RCC_CR_PLLRDY))
    ; // Wait for PLL ready

  // Configure flash for the final 64 MHz system clock before switching SYSCLK
  // to the phase-locked loop (PLL). The STM32 register header names the field
  // bit masks rather than the encoded wait-state values, so LATENCY_1 sets the
  // field to binary 010, which selects two wait states.
  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_1;

  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE);          // AHB = /1, APB = /1
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_1; // SYSCLK = PLLRCLK

  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLLRCLK)
    ; // Wait for switch to PLL

  // Init Sys Tick after the system clock is at the frequency used by F_SYS_CLOCK.
  sys_tick_init(F_SYS_CLOCK / (1000U / (uint32_t)HAL_TICK_FREQ_1KHZ));
}

inline uint32_t get_sys_tick(void)
{
  uint32_t systick_inline = systick_global;
  return systick_inline;
}

inline uint32_t get_elapsed_seconds(void)
{
  uint32_t second_counter = second_counter_global;
  return second_counter;
}

void SysTick_Handler(void)
{
  // The global system tick, increments at 1Kz (every 1ms)
  systick_global++;

  // Increment second counter
  if (systick_global % 1000 == 0)
  {
    second_counter_global++;
  }
}
