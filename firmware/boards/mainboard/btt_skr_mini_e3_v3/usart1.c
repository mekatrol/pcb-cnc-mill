#include <stdint.h>

#include "clock.h"
#include "board_hal.h"
#include "usart_hal.h"

// USART1 uart baud rate
#define USART1_BAUD_RATE 115200

// USART1 RX buffer
#define USART1_RX_BUFFER_SIZE 2048
static uint8_t usart1_rx_buffer[USART1_RX_BUFFER_SIZE];

// USART1 TX buffer
#define USART1_TX_BUFFER_SIZE 2048
static uint8_t usart1_tx_buffer[USART1_TX_BUFFER_SIZE];

// SKR Mini E3 V3 EXP1_3 is PA9 / USART1_TX.
#define USART1_TX_PIN BIT_09_POS

// SKR Mini E3 V3 EXP1_5 is PA10 / USART1_RX.
#define USART1_RX_PIN BIT_10_POS

static usart_instance_t usart1_instance = {
    .hal = USART1,
    .baud_rate = USART_BRR(F_SYS_CLOCK, USART1_BAUD_RATE),

    .rx_buffer = usart1_rx_buffer,
    .rx_buffer_head = 0,
    .rx_buffer_tail = 0,
    .rx_buffer_size = USART1_RX_BUFFER_SIZE,

    .tx_buffer = usart1_tx_buffer,
    .tx_buffer_head = 0,
    .tx_buffer_tail = 0,
    .tx_buffer_size = USART1_TX_BUFFER_SIZE,

    /**/};

void diag_usart1_init()
{
  // Use APB/PCLK for the USART kernel clock so BRR matches F_SYS_CLOCK.
  RCC->CCIPR &= ~RCC_CCIPR_USART1SEL;

  usart_init_hal(
      &usart1_instance,      // The usart instance data
      GPIOA,                 // The GPIO port the usart is on
      USART1_TX_PIN,         // The GPIO TX pin
      USART1_RX_PIN,         // The GPIO rx pin
      &RCC->APBENR2,         // The USART peripheral clock enable register
      RCC_APBENR2_USART1EN,  // The USART enable bit
      USART1_IRQn            // The usart IRQ number
  );
}

void diag_send(uint8_t value)
{
  usart_send(&usart1_instance, value);
}

void diag_flush()
{
  usart_wait_data_sent(&usart1_instance);
}

void USART1_IRQHandler()
{
  usart_irq_handler(&usart1_instance);
}
