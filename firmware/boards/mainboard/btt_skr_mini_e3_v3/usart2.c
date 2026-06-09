#include <stdint.h>

#include "clock.h"
#include "board_hal.h"
#include "usart_hal.h"

// USART2 uart baud rate
#define USART2_BAUD_RATE 115200

// USART2 RX buffer
#define USART2_RX_BUFFER_SIZE 2048
static uint8_t usart2_rx_buffer[USART2_RX_BUFFER_SIZE];

// USART2 TX buffer
#define USART2_TX_BUFFER_SIZE 2048
static uint8_t usart2_tx_buffer[USART2_TX_BUFFER_SIZE];

static usart_instance_t usart2_instance = {
    .hal = USART2,
    .baud_rate = USART_BRR(F_SYS_CLOCK, USART2_BAUD_RATE),

    .rx_buffer = usart2_rx_buffer,
    .rx_buffer_head = 0,
    .rx_buffer_tail = 0,
    .rx_buffer_size = USART2_RX_BUFFER_SIZE,

    .tx_buffer = usart2_tx_buffer,
    .tx_buffer_head = 0,
    .tx_buffer_tail = 0,
    .tx_buffer_size = USART2_TX_BUFFER_SIZE,

    /**/};

void tft_usart2_init()
{
  // Use APB/PCLK for the USART kernel clock so BRR matches F_SYS_CLOCK.
  RCC->CCIPR &= ~RCC_CCIPR_USART2SEL;

  usart_init_hal(
      &usart2_instance,     // The usart instance data
      GPIOA,                // The GPIO port the usart is on
      BIT_02_POS,           // The GPIO TX pin
      BIT_03_POS,           // The GPIO rx pin
      &RCC->APBENR1,        // The USART peripheral clock enable register
      RCC_APBENR1_USART2EN, // The USART enable bit
      USART2_LPUART2_IRQn   // The usart IRQ number
  );
}

bool usart2_byte_available(void)
{
  return usart2_instance.rx_count > 0;
}

uint8_t usart2_read_byte(void)
{
  int16_t value = usart_read(&usart2_instance);
  return value < 0 ? 0 : (uint8_t)value;
}

bool usart2_transmit_ready(void)
{
  uint32_t next_tx_buffer_head = usart2_instance.tx_buffer_head + 1;

  if (next_tx_buffer_head >= usart2_instance.tx_buffer_size)
  {
    next_tx_buffer_head = 0;
  }

  return next_tx_buffer_head != usart2_instance.tx_buffer_tail;
}

bool usart2_transmit_space_available(uint8_t byte_count)
{
  uint32_t used;

  if (usart2_instance.tx_buffer_head >= usart2_instance.tx_buffer_tail)
  {
    used = usart2_instance.tx_buffer_head - usart2_instance.tx_buffer_tail;
  }
  else
  {
    used = usart2_instance.tx_buffer_size - usart2_instance.tx_buffer_tail +
           usart2_instance.tx_buffer_head;
  }

  // Leave one byte empty so head and tail equality continues to mean the ring
  // is empty. This lets a protocol frame be queued atomically when the caller
  // has already checked the whole frame length.
  return byte_count < (usart2_instance.tx_buffer_size - used);
}

void usart2_write_byte(uint8_t value)
{
  usart_send(&usart2_instance, value);
}

void USART2_IRQHandler()
{
  usart_irq_handler(&usart2_instance);
}
