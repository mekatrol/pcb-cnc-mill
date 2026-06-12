.syntax       unified
.cpu          cortex-m0plus
.thumb

.global       g_pfnVectors
.global       Reset_Handler
.global       TIM7_IRQHandler
.global       Default_Handler

.section      .isr_vector, "a", %progbits
.align        2
.type         g_pfnVectors, %object

g_pfnVectors:
.word         _estack
.word         Reset_Handler
.word         Default_Handler               // NMI
.word         HardFault_Handler
.word         0, 0, 0, 0
.word         0, 0, 0
.word         Default_Handler               // SVC
.word         0
.word         0
.word         Default_Handler               // PendSV
.word         SysTick_Handler

// STM32G0B1 external interrupt vectors. Only SysTick and TIM7 are used by the
// first EBB42 Gen2 LED smoke-test image; all other sources stop in the default
// handler if they are enabled by mistake.
.word         Default_Handler               // IRQ 0
.word         Default_Handler               // IRQ 1
.word         Default_Handler               // IRQ 2
.word         Default_Handler               // IRQ 3
.word         Default_Handler               // IRQ 4
.word         Default_Handler               // IRQ 5
.word         Default_Handler               // IRQ 6
.word         Default_Handler               // IRQ 7
.word         Default_Handler               // IRQ 8
.word         Default_Handler               // IRQ 9
.word         Default_Handler               // IRQ 10
.word         Default_Handler               // IRQ 11
.word         Default_Handler               // IRQ 12
.word         Default_Handler               // IRQ 13
.word         Default_Handler               // IRQ 14
.word         Default_Handler               // IRQ 15
.word         Default_Handler               // IRQ 16
.word         Default_Handler               // IRQ 17
.word         TIM7_IRQHandler               // IRQ 18: TIM7/LPTIM2
.word         Default_Handler               // IRQ 19
.word         Default_Handler               // IRQ 20
.word         Default_Handler               // IRQ 21
.word         Default_Handler               // IRQ 22
.word         Default_Handler               // IRQ 23
.word         Default_Handler               // IRQ 24
.word         Default_Handler               // IRQ 25
.word         Default_Handler               // IRQ 26
.word         Default_Handler               // IRQ 27
.word         Default_Handler               // IRQ 28
.word         Default_Handler               // IRQ 29
.word         Default_Handler               // IRQ 30
.word         Default_Handler               // IRQ 31

.size         g_pfnVectors, . - g_pfnVectors

.global       main
.type         main, %function

.section      .text.Reset_Handler
.type         Reset_Handler, %function
Reset_Handler:
   cpsid      i

   // DFU writes this image at the STM32 application base, 0x08000000. Set VTOR
   // from the linked vector symbol so startup remains explicit and debuggable.
   ldr        r0, =0xE000ED08
   ldr        r1, =g_pfnVectors
   str        r1, [r0]

   // Clear any SysTick state left by an earlier image before clock_init starts
   // the one-millisecond system tick used by the board clock helpers.
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

HardFault_Handler:
   b          .
