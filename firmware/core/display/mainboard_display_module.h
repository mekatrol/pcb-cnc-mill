#ifndef PCB_CNC_MILL_MAINBOARD_DISPLAY_MODULE_H
#define PCB_CNC_MILL_MAINBOARD_DISPLAY_MODULE_H

// Interface for a display panel that is wired directly to a mainboard and runs
// inside the mainboard firmware image. This is different from a standalone
// display board such as the BTT TFT35 E3, which has its own microcontroller,
// startup code, linker script, clock tree, scheduler instance, and firmware
// binary.

// Configure only the hardware owned by the attached display module: panel bus
// pins, chip-select pins, encoder inputs, button inputs, backlight, sounder, and
// LEDs. The mainboard HAL owns MCU clocks, reset-time safety output states,
// motion timers, communication peripherals, and the system monotonic timer.
void mainboard_display_module_initialize_hardware(void);

// Run bounded display and input service work for the attached module. In the
// current mainboard skeleton this is called directly from the main loop. When
// the mainboard entry point moves to the shared scheduler, this work should be
// split into scheduler tasks with display/input priority.
void mainboard_display_module_run_background_tasks(void);

// Run bounded local feedback work such as buzzer chirps or status LEDs. This is
// separate from display redraw work so future scheduler task tables can give it
// a different period or priority.
void mainboard_display_module_run_feedback_tasks(void);

#endif // PCB_CNC_MILL_MAINBOARD_DISPLAY_MODULE_H
