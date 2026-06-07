# pcb-cnc-mill
Firmware for a PCB milling CNC.

## Project Direction

This firmware is for hobby PCB mill control. It takes ideas from Klipper,
Marlin, and GRBL: host command handling, G-code parsing, planned motion,
precise step generation, spindle control, and clear machine safety states.

## Firmware Layout

Firmware is grouped by function first, then by hardware brand and model where
needed. This keeps shared control code separate from board pin maps and vendor
details.

- `firmware/core/` - hardware-neutral parser, command, state, safety, planner,
  and step scheduling code. G-code parsing, command dispatch, machine state,
  safety state, motion planning, and step scheduling belong here.
- `firmware/hal/include/` - standard HAL interfaces used by core and reusable
  drivers.
- `firmware/hal/stm32/` - STM32 implementations for GPIO, timers, PWM, serial,
  flash, ADC, interrupts, and other MCU-specific services.
- `firmware/hal/lpc/` - LPC implementations for the same standard HAL
  interfaces.
- `firmware/drivers/` - reusable device or protocol drivers that should be
  written once, then used by several boards or MCU families.
- `firmware/drivers/usb/` - shared USB device support.
- `firmware/drivers/usb_cdc/` - shared USB CDC serial support.
- `firmware/drivers/can/` - shared CAN support for boards and toolheads.
- `firmware/drivers/tmc2209/` - shared TMC2209 stepper driver support.
- `firmware/boards/` - board-specific support grouped by hardware function
  first, then by vendor and model at the leaf. Pin maps, MCU quirks, timer
  choices, ports, and board feature flags belong here.
- `firmware/boards/mainboard/` - main controller board definitions, such as
  `btt_skr_mini_e3_v3/`, `fysetc_spider_king_10/`, and `duet_3_mini_5_plus/`.
- `firmware/boards/display/` - display and pendant board definitions, such as
  `btt_tft35_e3/`.
- `firmware/boards/toolhead/` - remote toolhead and IO board definitions, such
  as `btt_ebb36/` and `btt_ebb42/`.
- `firmware/configs/machines/` - per-machine config for limits, travel, units,
  selected boards, spindle settings, and safe defaults. Machine limits should
  live in config, not hidden source constants.

Rule of thumb: core code should not know a board brand exists. Board folders
hold pins, ports, MCU quirks, and feature flags. HAL folders hold processor
family implementations. Driver folders hold standard protocol and device code
once. Machine config ties selected hardware to one CNC build.

## Codex Project Notes

Codex guidance lives in `.codex/`:

- `.codex/todo.md` - implementation backlog.
- `.codex/design.md` - firmware design and approach.
- `.codex/standards.md` - rules Codex must follow when making changes.

Important rule: every project change should keep this README current.

## Current Firmware Targets

- `firmware/boards/display/btt_tft35_e3/` - initial GD32F205 bring-up for the
  BTT TFT35 E3 display. It includes startup code, linker script, GDB script,
  and Makefile targets for ST-Link/OpenOCD flash and debug.
- `firmware/boards/mainboard/btt_skr_mini_e3_v3/` - initial STM32G0B1RET6
  bring-up skeleton for the BTT SKR Mini E3 V3 mainboard. It includes startup
  code, linker script, GDB script, and Makefile targets for ST-Link/OpenOCD
  flash and debug.
