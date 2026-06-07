# pcb-cnc-mill
Firmware for a PCB milling CNC.

## Project Direction

This firmware is for hobby PCB mill control. It takes ideas from Klipper,
Marlin, and GRBL: host command handling, G-code parsing, planned motion,
precise step generation, spindle control, and clear machine safety states.
The base execution model is bare metal plus interrupts plus a cooperative
scheduler. Hardware timers own precise step timing; the scheduler runs bounded
non-blocking work for communication, storage, display, input, and housekeeping.
One-shot work such as chirps, display commands, user input events, communication
messages, and motion command enqueueing runs through bounded priority queues.
Stepper pulse timing still stays in hardware timer code.

## Firmware Layout

Firmware is grouped by function first, then by hardware brand and model where
needed. This keeps shared control code separate from board pin maps and vendor
details.

- `firmware/core/` - hardware-neutral parser, command, state, safety, planner,
  and step scheduling code. G-code parsing, command dispatch, machine state,
  safety state, motion planning, and step scheduling belong here.
- `firmware/core/mainboard/main.c` - shared main controller firmware entry
  point. It calls the selected mainboard HAL instead of knowing board pins or
  MCU details.
- `firmware/core/display/main.c` - shared display firmware entry point. It
  calls the selected display board HAL instead of owning LCD, touch, or encoder
  pins directly.
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

Mainboard, display, and toolhead firmware should share the same runtime model:
common startup flow, cooperative task table, interrupt-to-task events, bounded
queues, and short critical sections. Board support selects the hardware details
and enabled features.

## Codex Project Notes

Codex guidance lives in `.codex/`:

- `.codex/todo.md` - implementation backlog.
- `.codex/design.md` - firmware design and approach.
- `.codex/standards.md` - rules Codex must follow when making changes.

Important rule: every project change should keep this README current.

## Make Commands

Run these commands from the repository root:

```sh
make -C firmware mainboard
make -C firmware display
make -C firmware flash BUILD_TARGET=mainboard
make -C firmware flash BUILD_TARGET=display
```

Run these commands from inside `firmware/`:

```sh
make mainboard
make display
make flash BUILD_TARGET=mainboard
make flash BUILD_TARGET=display
```

The default boards are `MAIN_BOARD_NAME=btt_skr_mini_e3_v3` and
`DISPLAY_BOARD_NAME=btt_tft35_e3`. Override them on the command line when
adding another board:

```sh
make -C firmware mainboard MAIN_BOARD_NAME=btt_skr_mini_e3_v3
make -C firmware display DISPLAY_BOARD_NAME=btt_tft35_e3
```

Check the selected build settings with:

```sh
make -C firmware print-config
make -C firmware print-config BUILD_TARGET=display
```

## Current Firmware Targets

- `firmware/Makefile` - shared firmware build entry point. It selects the main
  controller with `MAIN_BOARD_NAME` and the display controller with
  `DISPLAY_BOARD_NAME`. Use `make -C firmware mainboard` for the default
  mainboard build, `make -C firmware display` for the default display build, or
  override names on the command line.
- `firmware/boards/display/btt_tft35_e3/` - initial GD32F205 bring-up for the
  BTT TFT35 E3 display. It includes startup code, linker script, GDB script,
  and Makefile targets for ST-Link/OpenOCD flash and debug.
- `firmware/boards/mainboard/btt_skr_mini_e3_v3/` - initial STM32G0B1RET6
  bring-up skeleton for the BTT SKR Mini E3 V3 mainboard. It includes startup
  code, linker script, GDB script, and Makefile targets for ST-Link/OpenOCD
  flash and debug.
