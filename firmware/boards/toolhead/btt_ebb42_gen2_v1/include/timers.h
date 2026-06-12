#ifndef BTT_EBB42_GEN2_V1_TIMERS_H
#define BTT_EBB42_GEN2_V1_TIMERS_H

#include <stdbool.h>
#include <stdint.h>

void status_timer7_init(uint32_t interval_milliseconds, bool enable_interrupt);

#endif // BTT_EBB42_GEN2_V1_TIMERS_H
