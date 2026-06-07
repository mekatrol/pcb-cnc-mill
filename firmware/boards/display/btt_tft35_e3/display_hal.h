#ifndef BTT_TFT35_E3_DISPLAY_HAL_H
#define BTT_TFT35_E3_DISPLAY_HAL_H

#include <stdint.h>

void display_initialize_hardware(void);
uint32_t display_get_monotonic_milliseconds(void);
void display_run_background_tasks(void);
void display_run_buzzer_tasks(void);
void display_wait_for_scheduler_tick(void);

#endif // BTT_TFT35_E3_DISPLAY_HAL_H
