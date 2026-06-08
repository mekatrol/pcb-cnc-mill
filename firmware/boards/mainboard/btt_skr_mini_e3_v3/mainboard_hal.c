#include <stdbool.h>
#include <stdint.h>

#include "mainboard_hal.h"

// STM32G0 reset starts from the 16 MHz high-speed internal oscillator. This
// board HAL does not yet own full clock-tree setup, so keep the TFT serial
// baud calculation tied to that reset-time peripheral clock until the main
// system clock is brought up deliberately.
enum
{
  SYSTEM_CORE_CLOCK_HZ = 16000000,
  DISPLAY_SERIAL_BAUD = 115200,
  DISPLAY_SERIAL_TX_PIN = 2, // SKR Mini E3 V3 TFT header: PA2 / USART2_TX
  DISPLAY_SERIAL_RX_PIN = 3, // SKR Mini E3 V3 TFT header: PA3 / USART2_RX
  DISPLAY_SERIAL_HEARTBEAT_BYTE = 0xA5,
  DISPLAY_SERIAL_HEARTBEAT_IDLE_LOOPS = 50000,
};

#define BIT(n) (1u << (n))
#define MMIO32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE 0x40021000u
#define GPIOA_BASE 0x50000000u
#define USART2_BASE 0x40004400u

#define RCC_IOPENR MMIO32(RCC_BASE + 0x34u)
#define RCC_APBENR1 MMIO32(RCC_BASE + 0x3Cu)

#define GPIO_MODER(base) MMIO32((base) + 0x00u)
#define GPIO_OSPEEDR(base) MMIO32((base) + 0x08u)
#define GPIO_PUPDR(base) MMIO32((base) + 0x0Cu)
#define GPIO_AFRL(base) MMIO32((base) + 0x20u)

#define USART_CR1(base) MMIO32((base) + 0x00u)
#define USART_CR2(base) MMIO32((base) + 0x04u)
#define USART_CR3(base) MMIO32((base) + 0x08u)
#define USART_BRR(base) MMIO32((base) + 0x0Cu)
#define USART_ISR(base) MMIO32((base) + 0x1Cu)
#define USART_RDR(base) MMIO32((base) + 0x24u)
#define USART_TDR(base) MMIO32((base) + 0x28u)

static volatile uint32_t idle_counter;
static uint32_t next_display_serial_heartbeat_idle_count;

static void configure_gpio_alternate_function(uint32_t base, uint8_t pin, uint8_t alternate_function)
{
  const uint32_t mode_shift = (uint32_t)pin * 2u;
  const uint32_t alternate_shift = (uint32_t)pin * 4u;

  // MODER 0b10 selects the alternate-function mux. PA2/PA3 use AF1 for
  // USART2 on STM32G0B1, which is the MCU-facing serial port on the SKR Mini
  // E3 V3 TFT connector.
  GPIO_MODER(base) = (GPIO_MODER(base) & ~(0x3u << mode_shift)) | (0x2u << mode_shift);
  GPIO_OSPEEDR(base) = (GPIO_OSPEEDR(base) & ~(0x3u << mode_shift)) | (0x2u << mode_shift);
  GPIO_PUPDR(base) = (GPIO_PUPDR(base) & ~(0x3u << mode_shift)) | (0x1u << mode_shift);
  GPIO_AFRL(base) = (GPIO_AFRL(base) & ~(0xFu << alternate_shift)) |
                    ((uint32_t)alternate_function << alternate_shift);
}

static void initialize_display_serial_link(void)
{
  RCC_IOPENR |= BIT(0);    // GPIOAEN
  RCC_APBENR1 |= BIT(17);  // USART2EN
  (void)RCC_IOPENR;
  (void)RCC_APBENR1;

  configure_gpio_alternate_function(GPIOA_BASE, DISPLAY_SERIAL_TX_PIN, 1);
  configure_gpio_alternate_function(GPIOA_BASE, DISPLAY_SERIAL_RX_PIN, 1);

  USART_CR1(USART2_BASE) = 0;
  USART_CR2(USART2_BASE) = 0;
  USART_CR3(USART2_BASE) = 0;
  USART_BRR(USART2_BASE) = (SYSTEM_CORE_CLOCK_HZ + (DISPLAY_SERIAL_BAUD / 2u)) /
                           DISPLAY_SERIAL_BAUD;

  // Enable 8 data bits, no parity, one stop bit, transmitter, receiver, and
  // the USART peripheral itself. Error handling and receive buffering belong
  // in the later protocol/service layer; this bring-up exposes non-blocking
  // raw byte access only.
  USART_CR1(USART2_BASE) = BIT(0) | BIT(2) | BIT(3);
}

void mainboard_initialize_hardware(void)
{
  // Placeholder until STM32G0 clock, GPIO, timer, USB, and safety inputs are
  // brought up for the SKR Mini E3 V3 main controller board.
  initialize_display_serial_link();
  next_display_serial_heartbeat_idle_count = DISPLAY_SERIAL_HEARTBEAT_IDLE_LOOPS;
}

void mainboard_run_background_tasks(void)
{
  idle_counter++;

  // The mainboard skeleton does not yet have a scheduler-owned millisecond
  // clock. Until that exists, use the fast background loop as a coarse heartbeat
  // source and keep the serial write non-blocking so machine-control work can
  // take ownership of this loop later without inheriting a busy-wait.
  if ((int32_t)(idle_counter - next_display_serial_heartbeat_idle_count) >= 0 &&
      mainboard_display_serial_transmit_ready())
  {
    mainboard_display_serial_write_byte(DISPLAY_SERIAL_HEARTBEAT_BYTE);
    next_display_serial_heartbeat_idle_count = idle_counter + DISPLAY_SERIAL_HEARTBEAT_IDLE_LOOPS;
  }
}

bool mainboard_display_serial_byte_available(void)
{
  return (USART_ISR(USART2_BASE) & BIT(5)) != 0; // RXNE_RXFNE
}

uint8_t mainboard_display_serial_read_byte(void)
{
  return (uint8_t)USART_RDR(USART2_BASE);
}

bool mainboard_display_serial_transmit_ready(void)
{
  return (USART_ISR(USART2_BASE) & BIT(7)) != 0; // TXE_TXFNF
}

void mainboard_display_serial_write_byte(uint8_t value)
{
  USART_TDR(USART2_BASE) = value;
}
