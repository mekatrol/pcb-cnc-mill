.syntax unified
.cpu cortex-m0plus
.thumb

.global g_pfnVectors
.global Reset_Handler
.global Default_Handler

.section .isr_vector, "a", %progbits
.align 2
.type g_pfnVectors, %object

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word 0
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler

  /* STM32G0B1 interrupt vectors. Keep unused IRQs on the default handler. */
  .word WWDG_IRQHandler
  .word PVD_IRQHandler
  .word RTC_TAMP_IRQHandler
  .word FLASH_IRQHandler
  .word RCC_CRS_IRQHandler
  .word EXTI0_1_IRQHandler
  .word EXTI2_3_IRQHandler
  .word EXTI4_15_IRQHandler
  .word UCPD1_2_IRQHandler
  .word DMA1_Channel1_IRQHandler
  .word DMA1_Channel2_3_IRQHandler
  .word DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler
  .word ADC_COMP_IRQHandler
  .word TIM1_BRK_UP_TRG_COM_IRQHandler
  .word TIM1_CC_IRQHandler
  .word TIM2_IRQHandler
  .word TIM3_TIM4_IRQHandler
  .word TIM6_DAC_LPTIM1_IRQHandler
  .word TIM7_LPTIM2_IRQHandler
  .word TIM14_IRQHandler
  .word TIM15_IRQHandler
  .word TIM16_IRQHandler
  .word TIM17_IRQHandler
  .word I2C1_IRQHandler
  .word I2C2_IRQHandler
  .word SPI1_IRQHandler
  .word SPI2_IRQHandler
  .word USART1_IRQHandler
  .word USART2_IRQHandler
  .word USART3_4_LPUART1_IRQHandler
  .word CEC_CAN_IRQHandler
  .word USB_UCPD1_2_IRQHandler

.size g_pfnVectors, . - g_pfnVectors

.section .text.Reset_Handler, "ax", %progbits
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0

zero_bss:
  cmp r0, r1
  bcc zero_bss_word
  b copy_data_start

zero_bss_word:
  str r2, [r0]
  adds r0, r0, #4
  b zero_bss

copy_data_start:
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata

copy_data:
  cmp r0, r1
  bcc copy_data_word
  b call_main

copy_data_word:
  ldr r3, [r2]
  str r3, [r0]
  adds r0, r0, #4
  adds r2, r2, #4
  b copy_data

call_main:
  bl main
  b .

.size Reset_Handler, . - Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function
Default_Handler:
  b .

.size Default_Handler, . - Default_Handler

.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
.weak WWDG_IRQHandler
.thumb_set WWDG_IRQHandler, Default_Handler
.weak PVD_IRQHandler
.thumb_set PVD_IRQHandler, Default_Handler
.weak RTC_TAMP_IRQHandler
.thumb_set RTC_TAMP_IRQHandler, Default_Handler
.weak FLASH_IRQHandler
.thumb_set FLASH_IRQHandler, Default_Handler
.weak RCC_CRS_IRQHandler
.thumb_set RCC_CRS_IRQHandler, Default_Handler
.weak EXTI0_1_IRQHandler
.thumb_set EXTI0_1_IRQHandler, Default_Handler
.weak EXTI2_3_IRQHandler
.thumb_set EXTI2_3_IRQHandler, Default_Handler
.weak EXTI4_15_IRQHandler
.thumb_set EXTI4_15_IRQHandler, Default_Handler
.weak UCPD1_2_IRQHandler
.thumb_set UCPD1_2_IRQHandler, Default_Handler
.weak DMA1_Channel1_IRQHandler
.thumb_set DMA1_Channel1_IRQHandler, Default_Handler
.weak DMA1_Channel2_3_IRQHandler
.thumb_set DMA1_Channel2_3_IRQHandler, Default_Handler
.weak DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler
.thumb_set DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler, Default_Handler
.weak ADC_COMP_IRQHandler
.thumb_set ADC_COMP_IRQHandler, Default_Handler
.weak TIM1_BRK_UP_TRG_COM_IRQHandler
.thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler, Default_Handler
.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler, Default_Handler
.weak TIM2_IRQHandler
.thumb_set TIM2_IRQHandler, Default_Handler
.weak TIM3_TIM4_IRQHandler
.thumb_set TIM3_TIM4_IRQHandler, Default_Handler
.weak TIM6_DAC_LPTIM1_IRQHandler
.thumb_set TIM6_DAC_LPTIM1_IRQHandler, Default_Handler
.weak TIM7_LPTIM2_IRQHandler
.thumb_set TIM7_LPTIM2_IRQHandler, Default_Handler
.weak TIM14_IRQHandler
.thumb_set TIM14_IRQHandler, Default_Handler
.weak TIM15_IRQHandler
.thumb_set TIM15_IRQHandler, Default_Handler
.weak TIM16_IRQHandler
.thumb_set TIM16_IRQHandler, Default_Handler
.weak TIM17_IRQHandler
.thumb_set TIM17_IRQHandler, Default_Handler
.weak I2C1_IRQHandler
.thumb_set I2C1_IRQHandler, Default_Handler
.weak I2C2_IRQHandler
.thumb_set I2C2_IRQHandler, Default_Handler
.weak SPI1_IRQHandler
.thumb_set SPI1_IRQHandler, Default_Handler
.weak SPI2_IRQHandler
.thumb_set SPI2_IRQHandler, Default_Handler
.weak USART1_IRQHandler
.thumb_set USART1_IRQHandler, Default_Handler
.weak USART2_IRQHandler
.thumb_set USART2_IRQHandler, Default_Handler
.weak USART3_4_LPUART1_IRQHandler
.thumb_set USART3_4_LPUART1_IRQHandler, Default_Handler
.weak CEC_CAN_IRQHandler
.thumb_set CEC_CAN_IRQHandler, Default_Handler
.weak USB_UCPD1_2_IRQHandler
.thumb_set USB_UCPD1_2_IRQHandler, Default_Handler
