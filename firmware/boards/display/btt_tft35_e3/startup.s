.syntax unified
.cpu cortex-m3
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
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler

  /* STM32F207 interrupt vectors. Keep unused IRQs on the default handler. */
  .rept 82
  .word Default_Handler
  .endr

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
  ldr r0, =0xE000ED08
  ldr r1, =g_pfnVectors
  str r1, [r0]
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
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
