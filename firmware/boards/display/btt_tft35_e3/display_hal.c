#include <stdbool.h>
#include <stdint.h>

#include "circular_buffer.h"
#include "display_hal.h"
#include "display_render.h"
#include "registers.h"
#include "rs232_display_protocol.h"
#include "chirp.h"

enum
{
  SYSTEM_CORE_CLOCK_HZ = 120000000,
  SYSTEM_CORE_CLOCK_MHZ = 120,
  DELAY_LOOP_CYCLES_PER_US = 45,
  ENCODER_A_PIN = 8,   // Upstream BTT TFT35 V3.0: LCD_ENCA_PIN PA8
  ENCODER_B_PIN = 9,   // Upstream BTT TFT35 V3.0: LCD_ENCB_PIN PC9
  ENCODER_BTN_PIN = 8, // Upstream BTT TFT35 V3.0: LCD_BTN_PIN PC8
  ENCODER_EN_PIN = 6,  // Upstream BTT TFT35 V3.0: LCD_ENC_EN_PIN PC6
  LCD_BACKLIGHT_PIN = 12,
  KNOB_LED_PIN = 7, // Upstream BTT TFT35 E3 V3.0: KNOB_LED_COLOR_PIN PC7
  BUZZER_PIN = 13,
  TOUCH_CS_PIN = 6,    // Upstream BTT TFT35 V3.0: XPT2046_CS PE6
  TOUCH_SCK_PIN = 5,   // Upstream BTT TFT35 V3.0: XPT2046_SCK PE5
  TOUCH_MISO_PIN = 4,  // Upstream BTT TFT35 V3.0: XPT2046_MISO PE4
  TOUCH_MOSI_PIN = 3,  // Upstream BTT TFT35 V3.0: XPT2046_MOSI PE3
  TOUCH_PEN_PIN = 13,  // Upstream BTT TFT35 V3.0: XPT2046_TPEN PC13
  MAINBOARD_SERIAL_TX_PIN = 2, // TFT35 RS232 connector: PA2 / USART1_TX
  MAINBOARD_SERIAL_RX_PIN = 3, // TFT35 RS232 connector: PA3 / USART1_RX
  MAINBOARD_SERIAL_BAUD = 115200,
  MAINBOARD_SERIAL_RECEIVE_BYTES_PER_SERVICE = 32,
  MAINBOARD_SERIAL_RX_BUFFER_SIZE = 2048,
  MAINBOARD_SERIAL_LINK_TIMEOUT_MS = 1500,
  APB1_PERIPHERAL_CLOCK_HZ = SYSTEM_CORE_CLOCK_HZ / 2u,
  KNOB_LED_PIXELS = 2, // Upstream GD TFT35 E3 V3.0: NEOPIXEL_PIXELS 2
  // Match upstream Knob_LED.c's integer math with the 120 MHz APB1 timer.
  NEOPIXEL_TIMER_PERIOD_TICKS = 150,
  NEOPIXEL_T0H_TICKS = 21,
  NEOPIXEL_T1H_TICKS = 128,
  EXMC_ADDRESS_SETUP_TICKS = 15,
  EXMC_DATA_SETUP_TICKS = 255,
  SYSTICK_RELOAD_TICKS = SYSTEM_CORE_CLOCK_HZ / 1000u,
};

typedef enum
{
  LCD_CONTROLLER_UNKNOWN,
  LCD_CONTROLLER_ILI9488,
  LCD_CONTROLLER_NT35310,
  LCD_CONTROLLER_ST7796S,
} lcd_controller_t;

static volatile uint32_t monotonic_milliseconds;
static volatile int32_t encoder_position;
static volatile bool encoder_pressed;
static uint8_t encoder_state;
static uint8_t button_history = 0xFFu;
static bool button_reported_pressed;
static bool touch_reported_pressed;
static uint32_t last_activity_ms;
static uint32_t next_knob_led_update_ms;
static bool backlight_enabled;
static bool knob_led_enabled;
static uint8_t knob_led_rainbow_phase;
static runtime_chirp_t buzzer_chirp;
static display_link_state_t mainboard_link_state = DISPLAY_LINK_STATE_WAITING;
static uint32_t last_mainboard_heartbeat_ms;
static rs232_display_protocol_parser_t mainboard_serial_parser;
static circular_buffer_t mainboard_serial_rx_buffer;
static uint8_t mainboard_serial_rx_buffer_storage[MAINBOARD_SERIAL_RX_BUFFER_SIZE];

static void knob_led_set_enabled(bool enabled);

void SysTick_Handler(void)
{
  monotonic_milliseconds++;
}

static void delay_ms(uint32_t ms)
{
  const uint32_t end = monotonic_milliseconds + ms;
  while ((int32_t)(end - monotonic_milliseconds) > 0)
  {
  }
}

static void delay_cycles(volatile uint32_t cycles)
{
  while (cycles-- > 0)
  {
  }
}

static void delay_us_approx(uint32_t us)
{
  delay_cycles(us * DELAY_LOOP_CYCLES_PER_US);
}

static uint32_t display_get_monotonic_microseconds(void)
{
  uint32_t before_milliseconds;
  uint32_t current_tick_count;
  uint32_t after_milliseconds;

  // SysTick counts down from SYSTICK_RELOAD_TICKS - 1 to zero once per
  // millisecond. Read the millisecond counter before and after the down-counter
  // so a wrap during this function does not combine the old millisecond with
  // the new sub-millisecond position.
  do
  {
    before_milliseconds = monotonic_milliseconds;
    current_tick_count = SYST_CVR;
    after_milliseconds = monotonic_milliseconds;
  } while (before_milliseconds != after_milliseconds);

  const uint32_t elapsed_ticks = SYSTICK_RELOAD_TICKS - current_tick_count;
  const uint32_t elapsed_microseconds = elapsed_ticks / SYSTEM_CORE_CLOCK_MHZ;

  return before_milliseconds * 1000u + elapsed_microseconds;
}

static void interrupts_disable(void)
{
  __asm volatile("cpsid i" ::: "memory");
}

static void interrupts_enable(void)
{
  __asm volatile("cpsie i" ::: "memory");
}

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

static bool gpio_input_is_high(uint32_t base, uint8_t pin)
{
  return (GPIO_ISTAT(base) & BIT(pin)) != 0;
}

static void backlight_set(bool enabled)
{
  gpio_output_set(GPIOD_BASE, LCD_BACKLIGHT_PIN, enabled);
  backlight_enabled = enabled;
}

static void record_activity(void)
{
  last_activity_ms = monotonic_milliseconds;
  if (!backlight_enabled)
  {
    backlight_set(true);
  }
  if (!knob_led_enabled)
  {
    knob_led_set_enabled(true);
  }
}

static void backlight_check_idle_timeout(void)
{
  if (backlight_enabled && (uint32_t)(monotonic_milliseconds - last_activity_ms) >= 30000u)
  {
    backlight_set(false);
    knob_led_set_enabled(false);
  }
}

static void touch_cs_set(bool high)
{
  gpio_output_set(GPIOE_BASE, TOUCH_CS_PIN, high);
}

static void touch_sck_set(bool high)
{
  gpio_output_set(GPIOE_BASE, TOUCH_SCK_PIN, high);
}

static void touch_mosi_set(bool high)
{
  gpio_output_set(GPIOE_BASE, TOUCH_MOSI_PIN, high);
}

static bool touch_miso_is_high(void)
{
  return gpio_input_is_high(GPIOE_BASE, TOUCH_MISO_PIN);
}

static uint8_t touch_spi_transfer(uint8_t value)
{
  uint8_t received = 0;

  for (uint8_t bit = 0; bit < 8; bit++)
  {
    touch_mosi_set((value & 0x80u) != 0);
    value <<= 1;
    touch_sck_set(false);
    delay_us_approx(2);
    received <<= 1;
    if (touch_miso_is_high())
    {
      received |= 1u;
    }
    touch_sck_set(true);
    delay_us_approx(2);
  }

  return received;
}

static uint16_t touch_read_adc(uint8_t command)
{
  touch_cs_set(false);
  touch_spi_transfer(command);
  uint16_t value = touch_spi_transfer(0xFFu);
  value = (uint16_t)((value << 8) | touch_spi_transfer(0xFFu));
  touch_cs_set(true);

  return (uint16_t)(value >> 4);
}

static bool touch_is_pressed(void)
{
  return !gpio_input_is_high(GPIOC_BASE, TOUCH_PEN_PIN);
}

static bool touch_read_valid_sample(void)
{
  const uint16_t x = touch_read_adc(0xD0u);
  const uint16_t y = touch_read_adc(0x90u);

  return x > 80u && x < 4016u && y > 80u && y < 4016u;
}

static void knob_led_raw_set(bool high)
{
  if (high)
  {
    GPIO_BOP(GPIOC_BASE) = BIT(KNOB_LED_PIN);
  }
  else
  {
    GPIO_BC(GPIOC_BASE) = BIT(KNOB_LED_PIN);
  }
}

static void knob_led_write_bit(bool high_bit)
{
  const uint32_t high_ticks = high_bit ? NEOPIXEL_T1H_TICKS : NEOPIXEL_T0H_TICKS;

  TIMER_CNT(TIMER5_BASE) = 0;
  TIMER_INTF(TIMER5_BASE) = 0;
  knob_led_raw_set(true);
  while (TIMER_CNT(TIMER5_BASE) < high_ticks)
  {
  }
  knob_led_raw_set(false);
  while ((TIMER_INTF(TIMER5_BASE) & BIT(0)) == 0)
  {
  }
  TIMER_INTF(TIMER5_BASE) = 0;
}

static void knob_led_write_byte(uint8_t value)
{
  for (int8_t bit = 7; bit >= 0; bit--)
  {
    knob_led_write_bit((value & BIT(bit)) != 0);
  }
}

static void knob_led_write_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
  knob_led_write_byte(green);
  knob_led_write_byte(red);
  knob_led_write_byte(blue);
}

static void knob_led_begin_frame(void)
{
  interrupts_disable();
  TIMER_CTL0(TIMER5_BASE) |= BIT(0);
}

static void knob_led_end_frame(void)
{
  TIMER_CTL0(TIMER5_BASE) &= ~BIT(0);
  interrupts_enable();
  delay_us_approx(300);
}

static uint8_t scale_color(uint8_t value)
{
  return (uint8_t)(value >> 3);
}

static void rainbow_color(uint8_t phase, uint8_t *red, uint8_t *green, uint8_t *blue)
{
  if (phase < 85u)
  {
    *red = scale_color((uint8_t)(255u - phase * 3u));
    *green = scale_color((uint8_t)(phase * 3u));
    *blue = 0;
  }
  else if (phase < 170u)
  {
    phase = (uint8_t)(phase - 85u);
    *red = 0;
    *green = scale_color((uint8_t)(255u - phase * 3u));
    *blue = scale_color((uint8_t)(phase * 3u));
  }
  else
  {
    phase = (uint8_t)(phase - 170u);
    *red = scale_color((uint8_t)(phase * 3u));
    *green = 0;
    *blue = scale_color((uint8_t)(255u - phase * 3u));
  }
}

static void knob_led_show_solid(uint8_t red, uint8_t green, uint8_t blue)
{
  knob_led_begin_frame();
  for (uint8_t pixel = 0; pixel < KNOB_LED_PIXELS; pixel++)
  {
    knob_led_write_rgb(red, green, blue);
  }
  knob_led_end_frame();
}

static void knob_led_show_rainbow(void)
{
  knob_led_begin_frame();
  for (uint8_t pixel = 0; pixel < KNOB_LED_PIXELS; pixel++)
  {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    rainbow_color((uint8_t)(knob_led_rainbow_phase + pixel * 64u), &red, &green, &blue);
    knob_led_write_rgb(red, green, blue);
  }
  knob_led_end_frame();
}

static void knob_led_update_rainbow(void)
{
  if (!knob_led_enabled || (int32_t)(next_knob_led_update_ms - monotonic_milliseconds) > 0)
  {
    return;
  }

  knob_led_rainbow_phase = (uint8_t)(knob_led_rainbow_phase + 3u);
  knob_led_show_rainbow();
  next_knob_led_update_ms = monotonic_milliseconds + 80u;
}

static void knob_led_set_enabled(bool enabled)
{
  if (enabled)
  {
    knob_led_show_rainbow();
    next_knob_led_update_ms = monotonic_milliseconds + 80u;
  }
  else
  {
    knob_led_show_solid(0, 0, 0);
  }
  knob_led_enabled = enabled;
}

static void knob_led_timer_init(void)
{
  TIMER_CTL0(TIMER5_BASE) = 0;
  TIMER_PSC(TIMER5_BASE) = 0;
  TIMER_CAR(TIMER5_BASE) = NEOPIXEL_TIMER_PERIOD_TICKS - 1u;
  TIMER_CNT(TIMER5_BASE) = 0;
  TIMER_INTF(TIMER5_BASE) = 0;
  TIMER_CTL0(TIMER5_BASE) = BIT(0);
}

static void error_buzzer_pulse(void)
{
  for (uint8_t pulse = 0; pulse < 3; pulse++)
  {
    for (uint32_t edge = 0; edge < 260u; edge++)
    {
      gpio_output_set(GPIOD_BASE, BUZZER_PIN, (edge & 1u) != 0);
      delay_cycles(320u);
    }
    gpio_output_set(GPIOD_BASE, BUZZER_PIN, false);
    delay_ms(120);
  }
}

static void knob_led_show_error_forever(void)
{
  while (1)
  {
    knob_led_show_solid(80, 0, 0);
    error_buzzer_pulse();
    delay_ms(180);
    knob_led_show_solid(0, 0, 0);
    delay_ms(180);
  }
}

static void configure_system_clock_120mhz(void)
{
  // A bootloader may jump here with the PLL already driving SYSCLK. Move back
  // to the internal IRC8M oscillator and stop the PLL before changing its
  // source or multiplier; those fields are not safe to edit while PLL is on.
  RCU_CTL |= BIT(0); // IRC8MEN
  while ((RCU_CTL & BIT(1)) == 0) // IRC8MSTB
  {
  }

  RCU_CFG0 &= ~0x3u; // SCS = IRC8M
  while ((RCU_CFG0 & (0x3u << 2)) != 0) // SCSS = IRC8M
  {
  }

  RCU_CTL &= ~BIT(24); // PLLEN
  while ((RCU_CTL & BIT(25)) != 0) // PLLSTB
  {
  }

  FMC_WS = (FMC_WS & ~0x7u) | 0x2u;

  // AHB = SYSCLK, APB2 = AHB, APB1 = AHB / 2. This is the same 120 MHz
  // rate used by upstream GD32F20x CMSIS startup. Use IRC8M/2 for PLL input
  // because the LCD's EXMC bus remaps PD0/PD1 away from the HXTAL pins.
  RCU_CFG0 &= ~((0xFu << 4) | (0x7u << 8) | (0x7u << 11));
  RCU_CFG0 |= (4u << 8);

  // CK_PLL = IRC8M / 2 * 30 = 120 MHz. Keeping the PLL off HXTAL lets PD0/PD1
  // stay available for the 16-bit LCD data bus after AFIO remap.
  RCU_CFG0 &= ~(BIT(16) | BIT(17) | (0xFu << 18) | BIT(29));
  RCU_CFG0 |= BIT(29) | (13u << 18);

  RCU_CTL |= BIT(24); // PLLEN
  while ((RCU_CTL & BIT(25)) == 0)
  {
  }

  RCU_CFG0 = (RCU_CFG0 & ~0x3u) | 0x2u;
  while ((RCU_CFG0 & (0x3u << 2)) != (0x2u << 2))
  {
  }
}

static void initialize_clocks_for_display_board(void)
{
  configure_system_clock_120mhz();

  // RCU is GigaDevice's Reset and Clock Unit. These bits enable the Alternate
  // Function I/O block plus GPIO ports A, C, D, and E.
  RCU_APB2EN |= BIT(0) | BIT(2) | BIT(4) | BIT(5) | BIT(6);

  // Enable EXMC so the LCD can be written through the memory-mapped bus.
  RCU_AHB1EN |= BIT(8);
  RCU_APB1EN |= BIT(4) | // TIMER5 for NeoPixel bit timing
                BIT(17); // USART1 for the RS232 link to the mainboard
  (void)RCU_APB2EN;
  (void)RCU_AHB1EN;
  (void)RCU_APB1EN;

  SYST_RVR = SYSTICK_RELOAD_TICKS - 1u;
  SYST_CVR = 0;
  SYST_CSR = BIT(0) | BIT(1) | BIT(2);
  interrupts_enable();
  knob_led_timer_init();
}

static void initialize_mainboard_serial_link(void)
{
  // The TFT35 E3 RS232 connector is a board-level 5-wire serial link to the
  // mainboard. On the GD32F205 variant, that connector uses PA2/PA3 on
  // USART1, with normal 3.3 V UART signalling behind BTT's connector label.
  configure_gpio_pin_mode(GPIOA_BASE, MAINBOARD_SERIAL_TX_PIN, 0xBu);
  configure_gpio_pin_mode(GPIOA_BASE, MAINBOARD_SERIAL_RX_PIN, 0x8u);
  gpio_output_set(GPIOA_BASE, MAINBOARD_SERIAL_RX_PIN, true);

  USART_CTL0(USART1_BASE) = 0;
  USART_CTL1(USART1_BASE) = 0;
  USART_CTL2(USART1_BASE) = 0;
  USART_BAUD(USART1_BASE) = (APB1_PERIPHERAL_CLOCK_HZ + (MAINBOARD_SERIAL_BAUD / 2u)) /
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

static void initialize_display_board_gpio(void)
{
  // 0x3 is the GD32F20x GPIO mode for 50 MHz general-purpose push-pull output.
  configure_gpio_pin_mode(GPIOD_BASE, LCD_BACKLIGHT_PIN, 0x3);
  configure_gpio_pin_mode(GPIOD_BASE, BUZZER_PIN, 0x3);
  backlight_set(true);
  last_activity_ms = monotonic_milliseconds;
  gpio_output_set(GPIOD_BASE, BUZZER_PIN, false);

  configure_gpio_pin_mode(GPIOC_BASE, ENCODER_EN_PIN, 0x3);
  gpio_output_set(GPIOC_BASE, ENCODER_EN_PIN, true);
  configure_gpio_pin_mode(GPIOC_BASE, KNOB_LED_PIN, 0x3);
  knob_led_raw_set(false);
  delay_ms(1);
  knob_led_set_enabled(true);

  configure_gpio_pin_mode(GPIOE_BASE, TOUCH_CS_PIN, 0x3);
  configure_gpio_pin_mode(GPIOE_BASE, TOUCH_SCK_PIN, 0x3);
  configure_gpio_pin_mode(GPIOE_BASE, TOUCH_MOSI_PIN, 0x3);
  configure_gpio_pin_mode(GPIOE_BASE, TOUCH_MISO_PIN, 0x4);
  configure_gpio_pin_mode(GPIOC_BASE, TOUCH_PEN_PIN, 0x8);
  touch_cs_set(true);
  touch_sck_set(true);
  touch_mosi_set(true);
  gpio_output_set(GPIOC_BASE, TOUCH_PEN_PIN, true);

  // 0x8 is input mode with pull-up/pull-down. Setting the output latch high
  // selects pull-up on this STM32F1/GD32F20x-style GPIO peripheral.
  configure_gpio_pin_mode(GPIOA_BASE, ENCODER_A_PIN, 0x8);
  configure_gpio_pin_mode(GPIOC_BASE, ENCODER_B_PIN, 0x8);
  configure_gpio_pin_mode(GPIOC_BASE, ENCODER_BTN_PIN, 0x8);
  gpio_output_set(GPIOA_BASE, ENCODER_A_PIN, true);
  gpio_output_set(GPIOC_BASE, ENCODER_B_PIN, true);
  gpio_output_set(GPIOC_BASE, ENCODER_BTN_PIN, true);

  initialize_mainboard_serial_link();
}

static void configure_lcd_external_memory_bus_pin(uint32_t base, uint8_t pin)
{
  // 0xB is the GD32F20x GPIO mode for 50 MHz alternate-function push-pull.
  // The alternate function connects this pin to EXMC instead of plain GPIO.
  configure_gpio_pin_mode(base, pin, 0xBu);
}

static void initialize_lcd_parallel_external_memory_bus(void)
{
  const uint8_t port_d_lcd_bus_pins[] = {0, 1, 4, 5, 7, 8, 9, 10, 14, 15};
  const uint8_t port_e_lcd_bus_pins[] = {2, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  // PD0 and PD1 normally overlap oscillator pins. This remap connects them to
  // GPIO/EXMC so the LCD can use the full 16-bit parallel data bus.
  AFIO_PCF0 |= BIT(15);

  for (uint32_t i = 0; i < sizeof(port_d_lcd_bus_pins); i++)
  {
    configure_lcd_external_memory_bus_pin(GPIOD_BASE, port_d_lcd_bus_pins[i]);
  }

  for (uint32_t i = 0; i < sizeof(port_e_lcd_bus_pins); i++)
  {
    configure_lcd_external_memory_bus_pin(GPIOE_BASE, port_e_lcd_bus_pins[i]);
  }

  EXMC_SNCTL0 = 0;
  EXMC_SNTCFG0 = (EXMC_DATA_SETUP_TICKS << 8) |
                 (EXMC_ADDRESS_SETUP_TICKS << 0);
  EXMC_SNCTL0 = BIT(12) | // Write enable
                BIT(6) |  // NOR flash access enable, matching upstream EXMC mode
                BIT(4) |  // 16-bit data bus
                BIT(3) |  // NOR-like async device
                BIT(0);   // Bank enable
}

static void lcd_write_command(uint16_t command)
{
  LCD_REG = command;
}

static void lcd_write_data(uint16_t data)
{
  LCD_RAM = data;
}

static uint16_t lcd_read_data(void)
{
  return LCD_RAM;
}

static void lcd_command_data(uint16_t command, uint16_t data)
{
  lcd_write_command(command);
  lcd_write_data(data);
}

static uint16_t lcd_read_id_d3(void)
{
  lcd_write_command(0xD3u);
  (void)lcd_read_data(); // Dummy read
  (void)lcd_read_data();
  uint16_t id = lcd_read_data();
  id <<= 8;
  id |= lcd_read_data();
  return id;
}

static uint16_t lcd_read_id_d4(void)
{
  lcd_write_command(0xD4u);
  (void)lcd_read_data(); // Dummy read
  (void)lcd_read_data();
  uint16_t id = lcd_read_data();
  id <<= 8;
  id |= lcd_read_data();
  return id;
}

static lcd_controller_t lcd_detect_controller(void)
{
  const uint16_t d3_id = lcd_read_id_d3();
  if (d3_id == 0x9488u)
  {
    return LCD_CONTROLLER_ILI9488;
  }
  if (lcd_read_id_d4() == 0x5310u)
  {
    return LCD_CONTROLLER_NT35310;
  }
  if (d3_id == 0x7796u)
  {
    return LCD_CONTROLLER_ST7796S;
  }
  return LCD_CONTROLLER_UNKNOWN;
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  lcd_write_command(0x2Au);
  lcd_write_data(x0 >> 8);
  lcd_write_data(x0 & 0xFFu);
  lcd_write_data(x1 >> 8);
  lcd_write_data(x1 & 0xFFu);

  lcd_write_command(0x2Bu);
  lcd_write_data(y0 >> 8);
  lcd_write_data(y0 & 0xFFu);
  lcd_write_data(y1 >> 8);
  lcd_write_data(y1 & 0xFFu);

  lcd_write_command(0x2Cu);
}

static void lcd_fill(uint16_t color)
{
  lcd_set_window(0, 0, 479, 319);
  for (uint32_t i = 0; i < 480u * 320u; i++)
  {
    lcd_write_data(color);
  }
}

static void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  lcd_set_window(x, y, (uint16_t)(x + width - 1u), (uint16_t)(y + height - 1u));
  for (uint32_t i = 0; i < (uint32_t)width * height; i++)
  {
    lcd_write_data(color);
  }
}

static void lcd_surface_fill(void *context, uint16_t color)
{
  (void)context;
  lcd_fill(color);
}

static void lcd_surface_fill_rect(void *context, uint16_t x, uint16_t y, uint16_t width,
                                  uint16_t height, uint16_t color)
{
  (void)context;
  lcd_fill_rect(x, y, width, height, color);
}

static const display_surface_t lcd_surface = {
  .context = 0,
  .width_pixels = 480,
  .height_pixels = 320,
  .fill = lcd_surface_fill,
  .fill_rect = lcd_surface_fill_rect,
};

static void lcd_initialize_ili9488(void)
{
  lcd_write_command(0xC0u);
  lcd_write_data(0x0Cu);
  lcd_write_data(0x02u);
  lcd_command_data(0xC1u, 0x44u);
  lcd_write_command(0xC5u);
  lcd_write_data(0x00u);
  lcd_write_data(0x16u);
  lcd_write_data(0x80u);
  lcd_command_data(0x36u, 0x28u);
  lcd_command_data(0x3Au, 0x55u);
  lcd_command_data(0xB0u, 0x00u);
  lcd_command_data(0xB1u, 0xB0u);
  lcd_command_data(0xB4u, 0x02u);
  lcd_write_command(0xB6u);
  lcd_write_data(0x02u);
  lcd_write_data(0x02u);
  lcd_command_data(0xE9u, 0x00u);
  lcd_write_command(0xF7u);
  lcd_write_data(0xA9u);
  lcd_write_data(0x51u);
  lcd_write_data(0x2Cu);
  lcd_write_data(0x82u);
  lcd_write_command(0x11u);
  delay_ms(120);
  lcd_write_command(0x29u);
}

static void lcd_initialize_nt35310(void)
{
  lcd_write_command(0xC0u);
  lcd_write_data(0x0Cu);
  lcd_write_data(0x02u);
  lcd_command_data(0xC1u, 0x44u);
  lcd_write_command(0xC5u);
  lcd_write_data(0x00u);
  lcd_write_data(0x16u);
  lcd_write_data(0x80u);
  lcd_command_data(0x36u, 0x60u);
  lcd_command_data(0x3Au, 0x55u);
  lcd_command_data(0xB0u, 0x00u);
  lcd_command_data(0xB1u, 0xB0u);
  lcd_command_data(0xB4u, 0x02u);
  lcd_write_command(0xB6u);
  lcd_write_data(0x02u);
  lcd_write_data(0x02u);
  lcd_command_data(0xE9u, 0x00u);
  lcd_write_command(0xF7u);
  lcd_write_data(0xA9u);
  lcd_write_data(0x51u);
  lcd_write_data(0x2Cu);
  lcd_write_data(0x82u);
  lcd_write_command(0x11u);
  delay_ms(120);
  lcd_write_command(0x29u);
}

static void lcd_initialize_st7796s(void)
{
  lcd_write_command(0xC0u);
  lcd_write_data(0x0Cu);
  lcd_write_data(0x02u);
  lcd_command_data(0xC1u, 0x44u);
  lcd_write_command(0xC5u);
  lcd_write_data(0x00u);
  lcd_write_data(0x16u);
  lcd_write_data(0x80u);
  lcd_command_data(0x36u, 0x28u);
  lcd_command_data(0x3Au, 0x55u);
  lcd_command_data(0xB0u, 0x00u);
  lcd_command_data(0xB1u, 0xB0u);
  lcd_command_data(0xB4u, 0x02u);
  lcd_write_command(0xB6u);
  lcd_write_data(0x02u);
  lcd_write_data(0x02u);
  lcd_command_data(0xE9u, 0x00u);
  lcd_write_command(0xF7u);
  lcd_write_data(0xA9u);
  lcd_write_data(0x51u);
  lcd_write_data(0x2Cu);
  lcd_write_data(0x82u);
  lcd_write_command(0x11u);
  delay_ms(120);
  lcd_write_command(0x29u);
}

static bool lcd_initialize_detected_controller(void)
{
  const lcd_controller_t controller = lcd_detect_controller();

  switch (controller)
  {
  case LCD_CONTROLLER_ILI9488:
    lcd_initialize_ili9488();
    return true;
  case LCD_CONTROLLER_NT35310:
    lcd_initialize_nt35310();
    return true;
  case LCD_CONTROLLER_ST7796S:
    lcd_initialize_st7796s();
    return true;
  case LCD_CONTROLLER_UNKNOWN:
  default:
    return false;
  }
}

static void initialize_lcd_controller(void)
{
  backlight_set(true);
  initialize_lcd_parallel_external_memory_bus();
  delay_ms(20);

  lcd_write_command(0x01u); // Software reset
  delay_ms(120);
  if (!lcd_initialize_detected_controller())
  {
    knob_led_show_error_forever();
  }
  delay_ms(20);

  display_render_draw_home_screen(&lcd_surface, mainboard_link_state);
}

static void set_mainboard_link_state(display_link_state_t next_link_state)
{
  if (mainboard_link_state == next_link_state)
  {
    return;
  }

  mainboard_link_state = next_link_state;
  display_render_draw_home_link_state(&lcd_surface, mainboard_link_state);
}

static void service_mainboard_serial_link(void)
{
  for (uint8_t received_count = 0; received_count < MAINBOARD_SERIAL_RECEIVE_BYTES_PER_SERVICE;
       received_count++)
  {
    if (!display_mainboard_serial_byte_available())
    {
      break;
    }

    const rs232_display_protocol_message_t message = rs232_display_protocol_receive_byte(
        &mainboard_serial_parser,
        display_mainboard_serial_read_byte());
    if (message == RS232_DISPLAY_PROTOCOL_MESSAGE_HEARTBEAT)
    {
      last_mainboard_heartbeat_ms = monotonic_milliseconds;
      set_mainboard_link_state(DISPLAY_LINK_STATE_OK);
    }
  }

  if ((uint32_t)(monotonic_milliseconds - last_mainboard_heartbeat_ms) >=
      MAINBOARD_SERIAL_LINK_TIMEOUT_MS)
  {
    set_mainboard_link_state(DISPLAY_LINK_STATE_LOST);
  }
}

static void buzzer_set_output(bool high)
{
  gpio_output_set(GPIOD_BASE, BUZZER_PIN, high);
}

static void chirp(uint32_t frequency_hz, uint32_t duration_ms)
{
  runtime_chirp_request(&buzzer_chirp, frequency_hz, duration_ms);
}

static void initialize_buzzer_chirp(void)
{
  const runtime_chirp_driver_t driver = {
    .get_monotonic_microseconds = display_get_monotonic_microseconds,
    .set_output = buzzer_set_output,
  };

  runtime_chirp_initialize(&buzzer_chirp, driver);
}

void display_run_buzzer_tasks(void)
{
  runtime_chirp_service(&buzzer_chirp);
}

static uint8_t encoder_sample(void)
{
  const uint8_t a = gpio_input_is_high(GPIOA_BASE, ENCODER_A_PIN) ? 1u : 0u;
  const uint8_t b = gpio_input_is_high(GPIOC_BASE, ENCODER_B_PIN) ? 1u : 0u;
  return (uint8_t)((a << 1) | b);
}

static int8_t encoder_poll(void)
{
  static const int8_t transitions[16] = {
      0,
      -1,
      1,
      0,
      1,
      0,
      0,
      -1,
      -1,
      0,
      0,
      1,
      0,
      1,
      -1,
      0,
  };

  const uint8_t sample = encoder_sample();
  const uint8_t index = (uint8_t)((encoder_state << 2) | sample);
  const int8_t delta = transitions[index];
  encoder_position += delta;
  encoder_state = sample;
  if (delta != 0)
  {
    record_activity();
  }

  button_history = (uint8_t)((button_history << 1) |
                             (gpio_input_is_high(GPIOC_BASE, ENCODER_BTN_PIN) ? 1u : 0u));

  if (button_history == 0x00u && !button_reported_pressed)
  {
    button_reported_pressed = true;
    encoder_pressed = true;
    record_activity();
    chirp(2800, 35);
  }
  else if (button_history == 0xFFu)
  {
    button_reported_pressed = false;
    encoder_pressed = false;
  }

  return delta;
}

static void touch_poll(void)
{
  if (touch_is_pressed())
  {
    if (!touch_reported_pressed && touch_read_valid_sample())
    {
      touch_reported_pressed = true;
      record_activity();
      chirp(2200, 45);
    }
  }
  else if (touch_reported_pressed)
  {
    touch_reported_pressed = false;
  }
}

void display_initialize_hardware(void)
{
  initialize_clocks_for_display_board();
  initialize_display_board_gpio();
  initialize_buzzer_chirp();
  chirp(1800, 60);
  delay_ms(80);
  mainboard_link_state = DISPLAY_LINK_STATE_WAITING;
  last_mainboard_heartbeat_ms = monotonic_milliseconds;
  rs232_display_protocol_initialize_parser(&mainboard_serial_parser);
  initialize_lcd_controller();

  encoder_state = encoder_sample();
}

uint32_t display_get_monotonic_milliseconds(void)
{
  return monotonic_milliseconds;
}

void display_run_background_tasks(void)
{
  service_mainboard_serial_link();
  (void)encoder_poll();
  touch_poll();
  backlight_check_idle_timeout();
  knob_led_update_rainbow();
}

void display_wait_for_scheduler_tick(void)
{
  if (runtime_chirp_has_work(&buzzer_chirp))
  {
    return;
  }

  delay_ms(1);
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
