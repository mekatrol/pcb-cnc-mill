#ifndef BTT_EBB42_GEN2_V1_TOOLHEAD_HAL_H
#define BTT_EBB42_GEN2_V1_TOOLHEAD_HAL_H

#include "board_hal.h"
#include "clock.h"
#include "timers.h"

void toolhead_initialize_hardware(void);
void toolhead_wait_for_interrupt(void);

#endif // BTT_EBB42_GEN2_V1_TOOLHEAD_HAL_H
