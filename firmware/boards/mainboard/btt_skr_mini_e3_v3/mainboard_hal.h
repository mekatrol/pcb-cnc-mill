#ifndef BTT_SKR_MINI_E3_V3_MAINBOARD_HAL_H
#define BTT_SKR_MINI_E3_V3_MAINBOARD_HAL_H

#include <stdbool.h>
#include <stdint.h>

void mainboard_initialize_hardware(void);
void mainboard_run_background_tasks(void);
bool mainboard_display_serial_byte_available(void);
uint8_t mainboard_display_serial_read_byte(void);
bool mainboard_display_serial_transmit_ready(void);
void mainboard_display_serial_write_byte(uint8_t value);

#endif // BTT_SKR_MINI_E3_V3_MAINBOARD_HAL_H
