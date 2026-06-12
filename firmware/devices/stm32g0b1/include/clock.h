#include <stdbool.h>
#include <stdint.h>

#ifndef STM32G0B1_CLOCK_H
#define STM32G0B1_CLOCK_H

#define F_SYS_CLOCK 64000000UL

void clock_init(void);
uint32_t get_sys_tick(void);
uint32_t get_elapsed_seconds(void);

#endif // STM32G0B1_CLOCK_H
