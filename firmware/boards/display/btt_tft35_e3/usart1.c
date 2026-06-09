#include <stdbool.h>
#include <stdint.h>

#include "board_clock.h"
#include "circular_buffer.h"
#include "registers.h"

enum
{
  MAINBOARD_SERIAL_TX_PIN = 2, // TFT35 RS232 connector: PA2 / USART1_TX
  MAINBOARD_SERIAL_RX_PIN = 3, // TFT35 RS232 connector: PA3 / USART1_RX
  MAINBOARD_SERIAL_BAUD = 115200,
  MAINBOARD_SERIAL_RX_BUFFER_SIZE = 2048,
};

static circular_buffer_t mainboard_serial_rx_buffer;
static uint8_t mainboard_serial_rx_buffer_storage[MAINBOARD_SERIAL_RX_BUFFER_SIZE];

static void configure_gpio_pin_mode(uint32_t base, uint8_t pin, uint32_t config)
{
  volatile uint32_t *ctl = (pin < 8) ? &GPIO_CTL0(base) : &GPIO_CTL1(base);
  const uint8_t shift = (pin & 7u) * 4u;
  *ctl = (*ctl & ~(0xFu << shift)) | (config << shift);
}

static void gpio_output_set(uint32_t base, uint8_t pin, bool high)
{
  if (high)
  {
    GPIO_BOP(base) = BIT(pin);
  }
  else
  {
    GPIO_BC(base) = BIT(pin);
  }
}

void mainboard_serial_usart1_init(void)
{
  // The TFT35 E3 RS232 connector is a board-level 5-wire serial link to the
  // mainboard. On the GD32F205 variant, that connector uses PA2/PA3 on
  // USART1, with normal 3.3 V UART signalling behind BTT's connector label.
  RCU_APB1EN |= BIT(17);
  (void)RCU_APB1EN;

  configure_gpio_pin_mode(GPIOA_BASE, MAINBOARD_SERIAL_TX_PIN, 0xBu);
  configure_gpio_pin_mode(GPIOA_BASE, MAINBOARD_SERIAL_RX_PIN, 0x8u);
  gpio_output_set(GPIOA_BASE, MAINBOARD_SERIAL_RX_PIN, true);

  USART_CTL0(USART1_BASE) = 0;
  USART_CTL1(USART1_BASE) = 0;
  USART_CTL2(USART1_BASE) = 0;
  USART_BAUD(USART1_BASE) = (DISPLAY_APB1_PERIPHERAL_CLOCK_HZ + (MAINBOARD_SERIAL_BAUD / 2u)) /
                            MAINBOARD_SERIAL_BAUD;

  circular_buffer_init(
      &mainboard_serial_rx_buffer,
      mainboard_serial_rx_buffer_storage,
      sizeof(mainboard_serial_rx_buffer_storage));

  // Enable 8 data bits, no parity, one stop bit, transmitter, receiver,
  // receive-not-empty interrupt, and the USART peripheral itself. The interrupt
  // only moves raw bytes into a ring buffer; frame parsing stays in the normal
  // display background task.
  USART_CTL0(USART1_BASE) = BIT(13) | BIT(5) | BIT(3) | BIT(2);

  // USART1 on the GD32F205 APB1 serial block uses Cortex-M external IRQ 38.
  NVIC_ISER(1) = BIT(6);
}

bool display_mainboard_serial_byte_available(void)
{
  return circular_buffer_count(&mainboard_serial_rx_buffer) > 0;
}

uint8_t display_mainboard_serial_read_byte(void)
{
  uint8_t value = 0;
  (void)circular_buffer_read(&mainboard_serial_rx_buffer, &value, 1);
  return value;
}

bool display_mainboard_serial_transmit_ready(void)
{
  return (USART_STAT(USART1_BASE) & BIT(7)) != 0; // TBE
}

void display_mainboard_serial_write_byte(uint8_t value)
{
  USART_DATA(USART1_BASE) = value;
}

void USART1_IRQHandler(void)
{
  while ((USART_STAT(USART1_BASE) & BIT(5)) != 0) // RBNE
  {
    const uint8_t value = (uint8_t)USART_DATA(USART1_BASE);
    if (circular_buffer_count(&mainboard_serial_rx_buffer) <
        MAINBOARD_SERIAL_RX_BUFFER_SIZE - 1u)
    {
      (void)circular_buffer_write(&mainboard_serial_rx_buffer, &value, 1);
    }
  }
}
