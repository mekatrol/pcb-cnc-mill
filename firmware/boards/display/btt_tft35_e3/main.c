#include <stdbool.h>
#include <stdint.h>

#define BIT(n) (1u << (n))

#define MMIO32(addr) (*(volatile uint32_t *)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(addr))

#define PERIPH_BASE 0x40000000u
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000u)
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000u)
#define SYSTICK_BASE 0xE000E010u

#define AFIO_BASE (APB2PERIPH_BASE + 0x0000u)
#define RCU_BASE (AHB1PERIPH_BASE + 0x1000u)
#define GPIO_BASE (APB2PERIPH_BASE + 0x0800u)

#define AFIO_PCF0 MMIO32(AFIO_BASE + 0x04u)
#define RCU_AHB1EN MMIO32(RCU_BASE + 0x14u)
#define RCU_APB2EN MMIO32(RCU_BASE + 0x18u)

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

#define EXMC_BASE 0xA0000000u
#define EXMC_SNCTL0 MMIO32(EXMC_BASE + 0x00u)
#define EXMC_SNTCFG0 MMIO32(EXMC_BASE + 0x04u)

#define SYST_CSR MMIO32(SYSTICK_BASE + 0x00u)
#define SYST_RVR MMIO32(SYSTICK_BASE + 0x04u)
#define SYST_CVR MMIO32(SYSTICK_BASE + 0x08u)

#define LCD_REG MMIO16(0x60000000u)
#define LCD_RAM MMIO16(0x60020000u)

enum {
  ENCODER_A_PIN = 8,    // Upstream BTT TFT35 V3.0: LCD_ENCA_PIN PA8
  ENCODER_B_PIN = 9,    // Upstream BTT TFT35 V3.0: LCD_ENCB_PIN PC9
  ENCODER_BTN_PIN = 8,  // Upstream BTT TFT35 V3.0: LCD_BTN_PIN PC8
  ENCODER_EN_PIN = 6,   // Upstream BTT TFT35 V3.0: LCD_ENC_EN_PIN PC6
  LCD_BACKLIGHT_PIN = 12,
  BUZZER_PIN = 13,
};

static volatile uint32_t tick_ms;
static volatile int32_t encoder_position;
static volatile bool encoder_pressed;
static uint8_t encoder_state;
static uint8_t button_history = 0xFFu;
static bool button_reported_pressed;

void SysTick_Handler(void) {
  tick_ms++;
}

static void delay_ms(uint32_t ms) {
  const uint32_t end = tick_ms + ms;
  while ((int32_t)(end - tick_ms) > 0) {
  }
}

static void delay_cycles(volatile uint32_t cycles) {
  while (cycles-- > 0) {
  }
}

static void gpio_config(uint32_t base, uint8_t pin, uint32_t config) {
  volatile uint32_t *ctl = (pin < 8) ? &GPIO_CTL0(base) : &GPIO_CTL1(base);
  const uint8_t shift = (pin & 7u) * 4u;
  *ctl = (*ctl & ~(0xFu << shift)) | (config << shift);
}

static void gpio_output_set(uint32_t base, uint8_t pin, bool high) {
  if (high) {
    GPIO_BOP(base) = BIT(pin);
  } else {
    GPIO_BC(base) = BIT(pin);
  }
}

static bool gpio_input_is_high(uint32_t base, uint8_t pin) {
  return (GPIO_ISTAT(base) & BIT(pin)) != 0;
}

static void clock_init(void) {
  RCU_APB2EN |= BIT(0) | BIT(2) | BIT(4) | BIT(5) | BIT(6);  // AFIO, GPIOA/C/D/E
  RCU_AHB1EN |= BIT(8);                                      // EXMC
  (void)RCU_APB2EN;
  (void)RCU_AHB1EN;

  SYST_RVR = 8000u - 1u;  // Reset clock is GD32 IRC8M.
  SYST_CVR = 0;
  SYST_CSR = BIT(0) | BIT(1) | BIT(2);
}

static void board_gpio_init(void) {
  gpio_config(GPIOD_BASE, LCD_BACKLIGHT_PIN, 0x3);  // 50 MHz output push-pull
  gpio_config(GPIOD_BASE, BUZZER_PIN, 0x3);
  gpio_output_set(GPIOD_BASE, LCD_BACKLIGHT_PIN, false);
  gpio_output_set(GPIOD_BASE, BUZZER_PIN, false);

  gpio_config(GPIOC_BASE, ENCODER_EN_PIN, 0x3);
  gpio_output_set(GPIOC_BASE, ENCODER_EN_PIN, true);

  gpio_config(GPIOA_BASE, ENCODER_A_PIN, 0x8);  // Input with pull-up/down
  gpio_config(GPIOC_BASE, ENCODER_B_PIN, 0x8);
  gpio_config(GPIOC_BASE, ENCODER_BTN_PIN, 0x8);
  gpio_output_set(GPIOA_BASE, ENCODER_A_PIN, true);
  gpio_output_set(GPIOC_BASE, ENCODER_B_PIN, true);
  gpio_output_set(GPIOC_BASE, ENCODER_BTN_PIN, true);
}

static void fsmc_pin(uint32_t base, uint8_t pin) {
  gpio_config(base, pin, 0xBu);  // 50 MHz alternate-function push-pull
}

static void lcd_bus_init(void) {
  const uint8_t pd_pins[] = {0, 1, 4, 5, 7, 8, 9, 10, 11, 14, 15};
  const uint8_t pe_pins[] = {7, 8, 9, 10, 11, 12, 13, 14, 15};

  AFIO_PCF0 |= BIT(15);  // Remap PD0/PD1 from oscillator pins to GPIO/EXMC.

  for (uint32_t i = 0; i < sizeof(pd_pins); i++) {
    fsmc_pin(GPIOD_BASE, pd_pins[i]);
  }

  for (uint32_t i = 0; i < sizeof(pe_pins); i++) {
    fsmc_pin(GPIOE_BASE, pe_pins[i]);
  }

  EXMC_SNCTL0 = 0;
  EXMC_SNTCFG0 = (1u << 28) |  // Access mode B
                 (15u << 8) |  // DATAST: deliberately slow for LCD bring-up
                 (3u << 4) |   // ADDHLD
                 (3u << 0);    // ADDSET
  EXMC_SNCTL0 = BIT(12) |      // Write enable
                BIT(4) |       // 16-bit data bus, SRAM-like async device
                BIT(0);        // Bank enable
}

static void lcd_write_command(uint16_t command) {
  LCD_REG = command;
}

static void lcd_write_data(uint16_t data) {
  LCD_RAM = data;
}

static void lcd_command_data(uint16_t command, uint16_t data) {
  lcd_write_command(command);
  lcd_write_data(data);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
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

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return (uint16_t)(((uint16_t)(red & 0xF8u) << 8) |
                    ((uint16_t)(green & 0xFCu) << 3) |
                    ((uint16_t)blue >> 3));
}

static void lcd_fill(uint16_t color) {
  lcd_set_window(0, 0, 479, 319);
  for (uint32_t i = 0; i < 480u * 320u; i++) {
    lcd_write_data(color);
  }
}

static void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
  lcd_set_window(x, y, (uint16_t)(x + width - 1u), (uint16_t)(y + height - 1u));
  for (uint32_t i = 0; i < (uint32_t)width * height; i++) {
    lcd_write_data(color);
  }
}

static const uint8_t *font5x7(char ch) {
  static const uint8_t glyphs[][5] = {
      {0x00, 0x00, 0x00, 0x00, 0x00},  // space
      {0x3E, 0x51, 0x49, 0x45, 0x3E},  // 0
      {0x00, 0x42, 0x7F, 0x40, 0x00},  // 1
      {0x42, 0x61, 0x51, 0x49, 0x46},  // 2
      {0x21, 0x41, 0x45, 0x4B, 0x31},  // 3
      {0x18, 0x14, 0x12, 0x7F, 0x10},  // 4
      {0x27, 0x45, 0x45, 0x45, 0x39},  // 5
      {0x3C, 0x4A, 0x49, 0x49, 0x30},  // 6
      {0x01, 0x71, 0x09, 0x05, 0x03},  // 7
      {0x36, 0x49, 0x49, 0x49, 0x36},  // 8
      {0x06, 0x49, 0x49, 0x29, 0x1E},  // 9
      {0x7E, 0x11, 0x11, 0x11, 0x7E},  // A
      {0x7F, 0x49, 0x49, 0x49, 0x36},  // B
      {0x3E, 0x41, 0x41, 0x41, 0x22},  // C
      {0x7F, 0x41, 0x41, 0x22, 0x1C},  // D
      {0x7F, 0x49, 0x49, 0x49, 0x41},  // E
      {0x7F, 0x09, 0x09, 0x09, 0x01},  // F
      {0x3E, 0x41, 0x49, 0x49, 0x7A},  // G
      {0x7F, 0x08, 0x08, 0x08, 0x7F},  // H
      {0x00, 0x41, 0x7F, 0x41, 0x00},  // I
      {0x20, 0x40, 0x41, 0x3F, 0x01},  // J
      {0x7F, 0x08, 0x14, 0x22, 0x41},  // K
      {0x7F, 0x40, 0x40, 0x40, 0x40},  // L
      {0x7F, 0x02, 0x0C, 0x02, 0x7F},  // M
      {0x7F, 0x04, 0x08, 0x10, 0x7F},  // N
      {0x3E, 0x41, 0x41, 0x41, 0x3E},  // O
      {0x7F, 0x09, 0x09, 0x09, 0x06},  // P
      {0x3E, 0x41, 0x51, 0x21, 0x5E},  // Q
      {0x7F, 0x09, 0x19, 0x29, 0x46},  // R
      {0x46, 0x49, 0x49, 0x49, 0x31},  // S
      {0x01, 0x01, 0x7F, 0x01, 0x01},  // T
      {0x3F, 0x40, 0x40, 0x40, 0x3F},  // U
      {0x1F, 0x20, 0x40, 0x20, 0x1F},  // V
      {0x3F, 0x40, 0x38, 0x40, 0x3F},  // W
      {0x63, 0x14, 0x08, 0x14, 0x63},  // X
      {0x07, 0x08, 0x70, 0x08, 0x07},  // Y
      {0x61, 0x51, 0x49, 0x45, 0x43},  // Z
      {0x00, 0x36, 0x36, 0x00, 0x00},  // :
      {0x08, 0x08, 0x08, 0x08, 0x08},  // -
  };

  if (ch >= 'a' && ch <= 'z') {
    ch = (char)(ch - 'a' + 'A');
  }
  if (ch == ' ') {
    return glyphs[0];
  }
  if (ch >= '0' && ch <= '9') {
    return glyphs[1 + (uint8_t)(ch - '0')];
  }
  if (ch >= 'A' && ch <= 'Z') {
    return glyphs[11 + (uint8_t)(ch - 'A')];
  }
  if (ch == ':') {
    return glyphs[37];
  }
  if (ch == '-') {
    return glyphs[38];
  }
  return glyphs[0];
}

static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t scale) {
  const uint8_t *glyph = font5x7(ch);

  for (uint8_t col = 0; col < 6; col++) {
    const uint8_t bits = (col < 5) ? glyph[col] : 0;
    for (uint8_t row = 0; row < 8; row++) {
      const uint16_t color = (bits & BIT(row)) ? fg : bg;
      lcd_fill_rect((uint16_t)(x + col * scale), (uint16_t)(y + row * scale), scale, scale, color);
    }
  }
}

static void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg,
                          uint8_t scale) {
  while (*text != '\0') {
    lcd_draw_char(x, y, *text, fg, bg, scale);
    x = (uint16_t)(x + 6u * scale);
    text++;
  }
}

static void lcd_draw_boot_screen(void) {
  const uint16_t bg = rgb565(12, 18, 28);
  const uint16_t white = rgb565(255, 255, 255);
  const uint16_t amber = rgb565(255, 190, 48);
  const uint16_t cyan = rgb565(64, 210, 230);

  lcd_fill(bg);
  lcd_fill_rect(0, 0, 160, 24, rgb565(210, 48, 48));
  lcd_fill_rect(160, 0, 160, 24, rgb565(48, 180, 90));
  lcd_fill_rect(320, 0, 160, 24, rgb565(48, 96, 220));
  lcd_fill_rect(0, 296, 480, 24, amber);

  lcd_draw_text(54, 70, "PCB CNC MILL", white, bg, 4);
  lcd_draw_text(84, 128, "BTT TFT35 E3", cyan, bg, 3);
  lcd_draw_text(120, 174, "GD32F205", amber, bg, 3);
  lcd_draw_text(92, 238, "ENCODER BUTTON CHIRPS", bg, amber, 2);
}

static void lcd_init(void) {
  gpio_output_set(GPIOD_BASE, LCD_BACKLIGHT_PIN, true);
  lcd_bus_init();
  delay_ms(20);

  lcd_write_command(0x01u);  // Software reset
  delay_ms(120);
  lcd_write_command(0x11u);  // Sleep out
  delay_ms(120);

  lcd_command_data(0xF0u, 0xC3u);  // ILI9488 command-set unlock, harmless on many clones
  lcd_command_data(0xF0u, 0x96u);
  lcd_command_data(0xB4u, 0x01u);  // Display inversion control
  lcd_command_data(0x3Au, 0x55u);  // 16-bit RGB565 pixels
  lcd_command_data(0x36u, 0x28u);  // Landscape, BGR order
  lcd_command_data(0xB0u, 0x00u);  // Common to ILI9488/ST7796S command sets
  lcd_write_command(0xB6u);        // Display function control
  lcd_write_data(0x02u);
  lcd_write_data(0x02u);
  lcd_write_data(0x3Bu);
  lcd_write_command(0x21u);        // Inversion on: makes a working bus visibly non-white
  lcd_write_command(0x29u);        // Display on
  delay_ms(20);

  lcd_draw_boot_screen();
}

static void chirp(uint32_t frequency_hz, uint32_t duration_ms) {
  if (frequency_hz == 0 || duration_ms == 0) {
    return;
  }

  const uint32_t half_period_us = 1000000u / (frequency_hz * 2u);
  const uint32_t toggles = duration_ms * frequency_hz * 2u / 1000u;
  const uint32_t cycles = (half_period_us * 8u) / 3u;

  for (uint32_t i = 0; i < toggles; i++) {
    gpio_output_set(GPIOD_BASE, BUZZER_PIN, (i & 1u) != 0);
    delay_cycles(cycles);
  }
  gpio_output_set(GPIOD_BASE, BUZZER_PIN, false);
}

static uint8_t encoder_sample(void) {
  const uint8_t a = gpio_input_is_high(GPIOA_BASE, ENCODER_A_PIN) ? 1u : 0u;
  const uint8_t b = gpio_input_is_high(GPIOC_BASE, ENCODER_B_PIN) ? 1u : 0u;
  return (uint8_t)((a << 1) | b);
}

static void encoder_poll(void) {
  static const int8_t transitions[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  const uint8_t sample = encoder_sample();
  const uint8_t index = (uint8_t)((encoder_state << 2) | sample);
  encoder_position += transitions[index];
  encoder_state = sample;

  button_history = (uint8_t)((button_history << 1) |
                             (gpio_input_is_high(GPIOC_BASE, ENCODER_BTN_PIN) ? 1u : 0u));

  if (button_history == 0x00u && !button_reported_pressed) {
    button_reported_pressed = true;
    encoder_pressed = true;
    chirp(2800, 35);
  } else if (button_history == 0xFFu) {
    button_reported_pressed = false;
    encoder_pressed = false;
  }
}

int main(void) {
  clock_init();
  board_gpio_init();
  chirp(1800, 60);
  delay_ms(80);
  lcd_init();

  encoder_state = encoder_sample();

  while (1) {
    encoder_poll();
    delay_ms(1);
  }
}
