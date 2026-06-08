#ifndef BTT_TFT35_E3_DISPLAY_HAL_H
#define BTT_TFT35_E3_DISPLAY_HAL_H

#include <stdbool.h>
#include <stdint.h>

void display_initialize_hardware(void);
uint32_t display_get_monotonic_milliseconds(void);
void display_run_background_tasks(void);
void display_run_buzzer_tasks(void);
void display_wait_for_scheduler_tick(void);
bool display_mainboard_serial_byte_available(void);
uint8_t display_mainboard_serial_read_byte(void);
bool display_mainboard_serial_transmit_ready(void);
void display_mainboard_serial_write_byte(uint8_t value);

#endif // BTT_TFT35_E3_DISPLAY_HAL_H
