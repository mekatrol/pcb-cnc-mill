# STM32G0B1 Device Support

This directory contains processor-family code shared by boards using an
STM32G0B1 microcontroller.

Shared here:

- CMSIS core and STM32G0B1 register definitions.
- Reset handling and the interrupt vector table.
- A linker script parameterized by the board flash layout.
- The current 8 MHz external-crystal to 64 MHz system-clock setup.
- Generic timer register setup, period changes, and reset-time delays.

Kept in each board directory:

- GPIO pin assignments and safe output states.
- Selection of which timer performs each board function.
- Interrupt behavior such as toggling a board LED or servicing a stepper.
- Bootloader offsets, DFU/ST-Link commands, and board-specific drivers.

The shared startup file uses weak interrupt handlers. A board or driver
overrides only the handlers for peripherals it enables.
