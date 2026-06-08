#include <stdbool.h>
#include <stdint.h>

#include "display_hal.h"
#include "registers.h"
#include "runtime/chirp.h"

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
static uint8_t color_bar_rotation;
static uint32_t last_activity_ms;
static uint32_t next_knob_led_update_ms;
static bool backlight_enabled;
static bool knob_led_enabled;
static uint8_t knob_led_rainbow_phase;
static runtime_chirp_t buzzer_chirp;

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
  RCU_APB1EN |= BIT(4); // TIMER5 for NeoPixel bit timing
  (void)RCU_APB2EN;
  (void)RCU_AHB1EN;
  (void)RCU_APB1EN;

  SYST_RVR = SYSTICK_RELOAD_TICKS - 1u;
  SYST_CVR = 0;
  SYST_CSR = BIT(0) | BIT(1) | BIT(2);
  interrupts_enable();
  knob_led_timer_init();
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

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
  return (uint16_t)(((uint16_t)(red & 0xF8u) << 8) |
                    ((uint16_t)(green & 0xFCu) << 3) |
                    ((uint16_t)blue >> 3));
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

static const uint8_t *font5x7(char ch)
{
  static const uint8_t glyphs[][5] = {
      {0x00, 0x00, 0x00, 0x00, 0x00}, // space
      {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
      {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
      {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
      {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
      {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
      {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
      {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
      {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
      {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
      {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
      {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
      {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
      {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
      {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
      {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
      {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
      {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
      {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
      {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
      {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
      {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
      {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
      {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
      {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
      {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
      {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
      {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
      {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
      {0x46, 0x49, 0x49, 0x49, 0x31}, // S
      {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
      {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
      {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
      {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
      {0x63, 0x14, 0x08, 0x14, 0x63}, // X
      {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
      {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
      {0x00, 0x36, 0x36, 0x00, 0x00}, // :
      {0x08, 0x08, 0x08, 0x08, 0x08}, // -
  };

  if (ch >= 'a' && ch <= 'z')
  {
    ch = (char)(ch - 'a' + 'A');
  }
  if (ch == ' ')
  {
    return glyphs[0];
  }
  if (ch >= '0' && ch <= '9')
  {
    return glyphs[1 + (uint8_t)(ch - '0')];
  }
  if (ch >= 'A' && ch <= 'Z')
  {
    return glyphs[11 + (uint8_t)(ch - 'A')];
  }
  if (ch == ':')
  {
    return glyphs[37];
  }
  if (ch == '-')
  {
    return glyphs[38];
  }
  return glyphs[0];
}

static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t scale)
{
  const uint8_t *glyph = font5x7(ch);

  for (uint8_t col = 0; col < 6; col++)
  {
    const uint8_t bits = (col < 5) ? glyph[col] : 0;
    for (uint8_t row = 0; row < 8; row++)
    {
      const uint16_t color = (bits & BIT(row)) ? fg : bg;
      lcd_fill_rect((uint16_t)(x + col * scale), (uint16_t)(y + row * scale), scale, scale, color);
    }
  }
}

static void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg,
                          uint8_t scale)
{
  while (*text != '\0')
  {
    lcd_draw_char(x, y, *text, fg, bg, scale);
    x = (uint16_t)(x + 6u * scale);
    text++;
  }
}

static void lcd_draw_color_bars(void)
{
  const uint16_t colors[] = {
      rgb565(210, 48, 48),
      rgb565(48, 180, 90),
      rgb565(48, 96, 220),
      rgb565(255, 190, 48),
      rgb565(180, 72, 220),
      rgb565(40, 210, 210),
  };

  for (uint8_t segment = 0; segment < 6; segment++)
  {
    const uint8_t color_index = (uint8_t)((segment + color_bar_rotation) % 6u);
    lcd_fill_rect((uint16_t)(segment * 80u), 0, 80, 24, colors[color_index]);
  }
}

static void lcd_draw_touch_button(bool pressed)
{
  const uint16_t fill = pressed ? rgb565(64, 210, 230) : rgb565(255, 190, 48);
  const uint16_t text = pressed ? rgb565(12, 18, 28) : rgb565(12, 18, 28);

  lcd_fill_rect(98, 214, 284, 58, fill);
  lcd_fill_rect(102, 218, 276, 50, pressed ? rgb565(96, 235, 255) : rgb565(255, 205, 82));
  lcd_draw_text(150, 232, "TOUCH CHIRP", text, pressed ? rgb565(96, 235, 255) : rgb565(255, 205, 82), 2);
}

static void lcd_draw_boot_screen(void)
{
  const uint16_t bg = rgb565(12, 18, 28);
  const uint16_t white = rgb565(255, 255, 255);
  const uint16_t amber = rgb565(255, 190, 48);
  const uint16_t cyan = rgb565(64, 210, 230);

  lcd_fill(bg);
  lcd_draw_color_bars();
  lcd_fill_rect(0, 296, 480, 24, amber);

  lcd_draw_text(54, 26, "MEKATROL", white, bg, 4);
  lcd_draw_text(54, 70, "PCB CNC MILL", white, bg, 4);
  lcd_draw_text(84, 128, "BTT TFT35 E3", cyan, bg, 3);
  lcd_draw_text(72, 174, "DIAL ROTATES TOP RGB", amber, bg, 2);
  lcd_draw_touch_button(false);
  lcd_draw_text(92, 300, "ENCODER BUTTON CHIRPS", bg, amber, 2);
}

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

  lcd_draw_boot_screen();
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
      lcd_draw_touch_button(true);
      chirp(2200, 45);
    }
  }
  else if (touch_reported_pressed)
  {
    touch_reported_pressed = false;
    lcd_draw_touch_button(false);
  }
}

void display_initialize_hardware(void)
{
  initialize_clocks_for_display_board();
  initialize_display_board_gpio();
  initialize_buzzer_chirp();
  chirp(1800, 60);
  delay_ms(80);
  initialize_lcd_controller();

  encoder_state = encoder_sample();
}

uint32_t display_get_monotonic_milliseconds(void)
{
  return monotonic_milliseconds;
}

void display_run_background_tasks(void)
{
  const int8_t encoder_delta = encoder_poll();
  if (encoder_delta > 0)
  {
    color_bar_rotation = (uint8_t)((color_bar_rotation + 1u) % 6u);
    lcd_draw_color_bars();
  }
  else if (encoder_delta < 0)
  {
    color_bar_rotation = (uint8_t)((color_bar_rotation + 5u) % 6u);
    lcd_draw_color_bars();
  }
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
