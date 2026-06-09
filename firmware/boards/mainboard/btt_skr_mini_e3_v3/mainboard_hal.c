#include <stdbool.h>
#include <stdint.h>

#include "mainboard_hal.h"
#include "rs232_display_protocol.h"

enum
{
  DISPLAY_SERIAL_HEARTBEAT_INTERVAL_MS = 500,
  DIAGNOSTIC_HEARTBEAT_INTERVAL_MS = 5000,
};

static uint32_t last_display_serial_heartbeat_ms;
static uint32_t last_diagnostic_heartbeat_ms;

void diag_usart1_init();
void tft_usart2_init();
bool usart2_byte_available(void);
uint8_t usart2_read_byte(void);
bool usart2_transmit_ready(void);
bool usart2_transmit_space_available(uint8_t byte_count);
void usart2_write_byte(uint8_t value);
void tmc2209_uart4_init();
void tmc2209_tick(uint32_t elapsed_ms, uint32_t elapsed_sec);
void init_eeprom();
void i2c1_master_init();
void limits_init_hal();
void usb_init_board_hal();

static bool write_display_serial_frame(const uint8_t *frame, uint8_t frame_size)
{
  if (!usart2_transmit_space_available(frame_size))
  {
    return false;
  }

  for (uint8_t index = 0; index < frame_size; index++)
  {
    mainboard_display_serial_write_byte(frame[index]);
  }

  return true;
}

void init_gpio()
{
  // Enable GPIO ports A, B, C & D
  RCC->IOPENR |= (RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN);

  GPIO_SET_MODE(GPIOD, BIT_08_POS, MODER_OUT); // Set LED status (PD8) to output
  GPIO_SET_MODE(GPIOC, BIT_06_POS, MODER_OUT); // Set FAN 0 (PC6) to output
  GPIO_SET_MODE(GPIOB, BIT_15_POS, MODER_OUT); // Set FAN 2 (PB15) to output
  GPIO_SET_MODE(GPIOC, BIT_08_POS, MODER_OUT); // Set E0 heater (PC8) to output
}

void mainboard_initialize_hardware(void)
{
  // Intialise board clock configuration
  clock_init();

  // Intialise GPIO
  init_gpio();

  // Intialise timers
  timer6_init();                  // Timer 6
  status_timer7_init(1000, true); // Timer 7 toggles status LED PD8 every 1000 ms.
  stepper_timer14_init();         // Timer 14 used for stepper pulse timing

  // Diagnostics USART1 on EXP1 and TFT USART2 on the TFT header are separate
  // interrupt-buffered serial links. Keeping diagnostics off USART2 prevents
  // debug text from appearing in the display protocol stream.
  diag_usart1_init();
  tft_usart2_init();

  // TMC2209 usart initialisation
  tmc2209_uart4_init();

  // EEPROM init (on I2C1)
  init_eeprom();
  i2c1_master_init();

  // Enable interrupt delivery before starting board peripherals that report
  // hardware events through interrupt handlers. The USB board HAL configures
  // the pins, clock source, pull-up, peripheral interrupt mask, and NVIC entry;
  // the shared USB device state machine is initialized immediately after this
  // board HAL returns.
  interrupts_enable();
  limits_init_hal();
  usb_init_board_hal();

  last_display_serial_heartbeat_ms = get_sys_tick();
  last_diagnostic_heartbeat_ms = last_display_serial_heartbeat_ms;
  diag_print("pcb-cnc-mill mainboard diagnostics ready\r\n");
}

void mainboard_run_background_tasks(void)
{
  uint32_t current_millisecond_tick = get_sys_tick();

  // The TFT35 E3 display marks the link lost after 1.5 seconds without a valid
  // heartbeat frame. Send through the USART2 interrupt-buffered driver so the
  // TFT link owns its peripheral registers separately from diagnostics.
  if ((uint32_t)(current_millisecond_tick - last_display_serial_heartbeat_ms) >=
      DISPLAY_SERIAL_HEARTBEAT_INTERVAL_MS)
  {
    uint8_t heartbeat_frame[RS232_DISPLAY_PROTOCOL_HEARTBEAT_FRAME_SIZE];
    const uint8_t heartbeat_frame_size = rs232_display_protocol_build_heartbeat_frame(
        heartbeat_frame,
        sizeof(heartbeat_frame));

    if (write_display_serial_frame(heartbeat_frame, heartbeat_frame_size))
    {
      last_display_serial_heartbeat_ms = current_millisecond_tick;
    }
  }

  // Keep a slow diagnostics heartbeat during board bring-up so the EXP1
  // USART1 link can be tested without requiring a USB event, limit switch edge,
  // or stepper-driver transaction to produce log text.
  if ((uint32_t)(current_millisecond_tick - last_diagnostic_heartbeat_ms) >=
      DIAGNOSTIC_HEARTBEAT_INTERVAL_MS)
  {
    diag_print("mainboard alive\r\n");
    last_diagnostic_heartbeat_ms = current_millisecond_tick;
  }
}

bool mainboard_display_serial_byte_available(void)
{
  return usart2_byte_available();
}

uint8_t mainboard_display_serial_read_byte(void)
{
  return usart2_read_byte();
}

bool mainboard_display_serial_transmit_ready(void)
{
  return usart2_transmit_ready();
}

void mainboard_display_serial_write_byte(uint8_t value)
{
  usart2_write_byte(value);
}
