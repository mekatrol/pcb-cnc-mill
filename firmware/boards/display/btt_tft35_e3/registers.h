#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#define BIT(n) (1u << (n))

#define MMIO32(addr) (*(volatile uint32_t *)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(addr))

#define PERIPH_BASE 0x40000000u
#define APB1PERIPH_BASE PERIPH_BASE
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000u)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000u)
#define SYSTICK_BASE 0xE000E010u
#define NVIC_BASE 0xE000E100u

#define AFIO_BASE (APB2PERIPH_BASE + 0x0000u)
#define RCU_BASE (AHB1PERIPH_BASE + 0x1000u)
#define FMC_BASE (AHB1PERIPH_BASE + 0x2000u)
#define GPIO_BASE (APB2PERIPH_BASE + 0x0800u)
#define TIMER_BASE APB1PERIPH_BASE

#define RCU_CTL MMIO32(RCU_BASE + 0x00u)
#define RCU_CFG0 MMIO32(RCU_BASE + 0x04u)
#define RCU_CFG1 MMIO32(RCU_BASE + 0x2Cu)
#define AFIO_PCF0 MMIO32(AFIO_BASE + 0x04u)
#define RCU_AHB1EN MMIO32(RCU_BASE + 0x14u)
#define RCU_APB2EN MMIO32(RCU_BASE + 0x18u)
#define RCU_APB1EN MMIO32(RCU_BASE + 0x1Cu)
#define FMC_WS MMIO32(FMC_BASE + 0x00u)

#define GPIOA_BASE (GPIO_BASE + 0x0000u)
#define GPIOC_BASE (GPIO_BASE + 0x0800u)
#define GPIOD_BASE (GPIO_BASE + 0x0C00u)
#define GPIOE_BASE (GPIO_BASE + 0x1000u)

#define GPIO_CTL0(base) MMIO32((base) + 0x00u)
#define GPIO_CTL1(base) MMIO32((base) + 0x04u)
#define GPIO_ISTAT(base) MMIO32((base) + 0x08u)
#define GPIO_OCTL(base) MMIO32((base) + 0x0Cu)
#define GPIO_BOP(base) MMIO32((base) + 0x10u)
#define GPIO_BC(base) MMIO32((base) + 0x14u)

#define TIMER5_BASE (TIMER_BASE + 0x1000u)
#define TIMER_CTL0(base) MMIO32((base) + 0x00u)
#define TIMER_INTF(base) MMIO32((base) + 0x10u)
#define TIMER_CNT(base) MMIO32((base) + 0x24u)
#define TIMER_PSC(base) MMIO32((base) + 0x28u)
#define TIMER_CAR(base) MMIO32((base) + 0x2Cu)

#define USART1_BASE (APB1PERIPH_BASE + 0x4400u)
#define USART_STAT(base) MMIO32((base) + 0x00u)
#define USART_DATA(base) MMIO32((base) + 0x04u)
#define USART_BAUD(base) MMIO32((base) + 0x08u)
#define USART_CTL0(base) MMIO32((base) + 0x0Cu)
#define USART_CTL1(base) MMIO32((base) + 0x10u)
#define USART_CTL2(base) MMIO32((base) + 0x14u)

// External Memory Controller. On GD32F205 it is the peripheral
// that drives the TFT LCD as a 16-bit 8080-style parallel memory device.
#define EXMC_BASE 0xA0000000u
#define EXMC_SNCTL0 MMIO32(EXMC_BASE + 0x00u)
#define EXMC_SNTCFG0 MMIO32(EXMC_BASE + 0x04u)

#define SYST_CSR MMIO32(SYSTICK_BASE + 0x00u)
#define SYST_RVR MMIO32(SYSTICK_BASE + 0x04u)
#define SYST_CVR MMIO32(SYSTICK_BASE + 0x08u)
#define NVIC_ISER(index) MMIO32(NVIC_BASE + ((index) * 4u))

// The LCD controller is memory-mapped through EXMC bank 0. These addresses
// match BTT's GD32F20x HAL mapping for PE2/FSMC_A23 as the LCD RS line.
#define LCD_REG MMIO16(0x60FFFFFEu)
#define LCD_RAM MMIO16(0x61000000u)

#endif // __REGISTERS_H__
