.syntax       unified
.cpu          cortex-m0plus
.thumb

.global       g_pfnVectors
.global       Reset_Handler
.global       Default_Handler

.section      .isr_vector, "a", %progbits
.align        2
.type         g_pfnVectors, %object

// Shared STM32G0B1 vector table. Every interrupt name has a weak default below.
// A board or device driver provides a strong C handler only for the peripherals
// it enables, so mainboards and toolheads can use this same startup file without
// copying the processor vector layout into each board directory.
g_pfnVectors:
.word         _estack
.word         Reset_Handler
.word         NMI_Handler
.word         HardFault_Handler
.word         0, 0, 0, 0
.word         0, 0, 0
.word         SVC_Handler
.word         0
.word         0
.word         PendSV_Handler
.word         SysTick_Handler

.word         WWDG_IRQHandler                    // IRQ 0
.word         PVD_VDDIO2_IRQHandler              // IRQ 1
.word         RTC_TAMP_IRQHandler                // IRQ 2
.word         FLASH_IRQHandler                   // IRQ 3
.word         RCC_CRS_IRQHandler                 // IRQ 4
.word         EXTI0_1_IRQHandler                 // IRQ 5
.word         EXTI2_3_IRQHandler                 // IRQ 6
.word         EXTI4_15_IRQHandler                // IRQ 7
.word         USB_UCPD1_2_IRQHandler             // IRQ 8
.word         DMA1_Channel1_IRQHandler           // IRQ 9
.word         DMA1_Channel2_3_IRQHandler         // IRQ 10
.word         DMA1_Ch4_7_DMA2_Ch1_5_IRQHandler   // IRQ 11
.word         ADC1_COMP_IRQHandler               // IRQ 12
.word         TIM1_BRK_UP_TRG_COM_IRQHandler     // IRQ 13
.word         TIM1_CC_IRQHandler                 // IRQ 14
.word         TIM2_IRQHandler                    // IRQ 15
.word         TIM3_TIM4_IRQHandler               // IRQ 16
.word         TIM6_DAC_IRQHandler                // IRQ 17
.word         TIM7_IRQHandler                    // IRQ 18
.word         TIM14_IRQHandler                   // IRQ 19
.word         TIM15_IRQHandler                   // IRQ 20
.word         TIM16_IRQHandler                   // IRQ 21
.word         TIM17_IRQHandler                   // IRQ 22
.word         I2C1_IRQHandler                    // IRQ 23
.word         I2C2_IRQHandler                    // IRQ 24
.word         SPI1_IRQHandler                    // IRQ 25
.word         SPI2_IRQHandler                    // IRQ 26
.word         USART1_IRQHandler                  // IRQ 27
.word         USART2_IRQHandler                  // IRQ 28
.word         USART3_4_LPUART1_IRQHandler        // IRQ 29
.word         CEC_CAN_IRQHandler                 // IRQ 30
.word         USB_UCPD2_IRQHandler               // IRQ 31

.size         g_pfnVectors, . - g_pfnVectors

.global       main
.type         main, %function

.section      .text.Reset_Handler
.type         Reset_Handler, %function
Reset_Handler:
   cpsid      i

   // The linker places this vector table at the selected application origin.
   // That may be 0x08000000 for direct/DFU images or a board bootloader offset.
   ldr        r0, =0xE000ED08
   ldr        r1, =g_pfnVectors
   str        r1, [r0]

   // Remove SysTick state inherited from a bootloader before the shared clock
   // setup starts the one-millisecond device tick.
   ldr        r0, =0xE000E010
   movs       r1, #0
   str        r1, [r0]
   str        r1, [r0, #4]
   str        r1, [r0, #8]

   ldr        r0, =_sbss
   ldr        r1, =_ebss
   movs       r2, #0

zero_bss:
   cmp        r0, r1
   bcc        zero_bss_word
   b          copy_data_setup

zero_bss_word:
   str        r2, [r0]
   adds       r0, r0, #4
   b          zero_bss

copy_data_setup:
   ldr        r0, =_sdata
   ldr        r1, =_edata
   ldr        r2, =_sidata

copy_data:
   cmp        r0, r1
   bcc        copy_data_word
   b          call_main

copy_data_word:
   ldr        r3, [r2]
   str        r3, [r0]
   adds       r0, r0, #4
   adds       r2, r2, #4
   b          copy_data

call_main:
   bl         main
   b          .

Default_Handler:
   b          .

// Weak aliases let each board override only the interrupt handlers it owns.
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
.weak PVD_VDDIO2_IRQHandler
.thumb_set PVD_VDDIO2_IRQHandler, Default_Handler
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
.weak USB_UCPD1_2_IRQHandler
.thumb_set USB_UCPD1_2_IRQHandler, Default_Handler
.weak DMA1_Channel1_IRQHandler
.thumb_set DMA1_Channel1_IRQHandler, Default_Handler
.weak DMA1_Channel2_3_IRQHandler
.thumb_set DMA1_Channel2_3_IRQHandler, Default_Handler
.weak DMA1_Ch4_7_DMA2_Ch1_5_IRQHandler
.thumb_set DMA1_Ch4_7_DMA2_Ch1_5_IRQHandler, Default_Handler
.weak ADC1_COMP_IRQHandler
.thumb_set ADC1_COMP_IRQHandler, Default_Handler
.weak TIM1_BRK_UP_TRG_COM_IRQHandler
.thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler, Default_Handler
.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler, Default_Handler
.weak TIM2_IRQHandler
.thumb_set TIM2_IRQHandler, Default_Handler
.weak TIM3_TIM4_IRQHandler
.thumb_set TIM3_TIM4_IRQHandler, Default_Handler
.weak TIM6_DAC_IRQHandler
.thumb_set TIM6_DAC_IRQHandler, Default_Handler
.weak TIM7_IRQHandler
.thumb_set TIM7_IRQHandler, Default_Handler
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
.weak USB_UCPD2_IRQHandler
.thumb_set USB_UCPD2_IRQHandler, Default_Handler
