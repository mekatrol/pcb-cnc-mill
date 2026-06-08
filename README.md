# pcb-cnc-mill
Firmware for a PCB milling CNC.

## Project Direction

This firmware is for hobby PCB mill control. It focuses on host command
handling, G-code parsing, planned motion, precise step generation, spindle
control, and clear machine safety states. The base execution model is bare
metal plus interrupts plus a priority-based scheduler. Hardware timers own
precise step timing; preemptive-priority scheduler tasks handle urgent bounded
service work such as planner refill and step generator starvation checks, and
normal scheduler tasks handle communication, storage, display, input, and
housekeeping. One-shot work such as chirps, display commands, user input
events, communication messages, and motion command enqueueing runs through
bounded priority queues. Stepper pulse timing still stays in hardware timer
code.

## Firmware Layout

Firmware is grouped by function first, then by hardware brand and model where
needed. This keeps shared control code separate from board pin maps and vendor
details.

- `firmware/core/` - hardware-neutral parser, command, state, safety, planner,
  runtime, and step scheduling code. G-code parsing, command dispatch, machine
  state, safety state, motion planning, shared runtime, and step scheduling
  belong here.
- `firmware/core/runtime/` - shared bare-metal runtime code used by mainboard,
  display, and toolhead builds. Priority scheduler dispatch and reusable
  low-priority chirp feedback state machines live here, not in board-role entry
  points.
- `firmware/core/mainboard/main.c` - shared main controller firmware entry
  point. It calls the selected mainboard HAL instead of knowing board pins or
  MCU details. It should only wire startup, runtime setup, and the main loop.
- `firmware/core/display/main.c` - shared display firmware entry point. It
  calls the selected display board HAL instead of owning LCD, touch, or encoder
  pins directly. It should only wire startup, runtime setup, and the main loop.
- `firmware/core/display/mainboard_display_module.h` - shared contract for a
  display panel compiled into a mainboard firmware image. Use this for panels
  such as a Mini12864 connected to a mainboard expansion header, not for
  display boards that have their own MCU and firmware image.
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
  choices, ports, and board feature flags belong here. Board support source
  files should use feature names such as `display_hal.c` or
  `mainboard_hal.c`; `main.c` is reserved for entry points.
- `firmware/boards/mainboard/` - main controller board definitions, such as
  `btt_skr_mini_e3_v3/`, `fysetc_spider_king_10/`, and `duet_3_mini_5_plus/`.
- `firmware/boards/display/` - display and pendant board definitions, such as
  `btt_tft35_e3/`. These boards have their own MCU, startup code, linker
  script, scheduler entry point, and firmware binary.
- `firmware/boards/display_module/` - mainboard-attached display module
  definitions, such as `btt_mini12864/`. These modules are compiled into the
  selected mainboard firmware image and do not own clocks, startup code,
  linker scripts, or separate firmware binaries.
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
common startup flow, priority task table, optional preemptive dispatch for
urgent priorities, interrupt-to-task events, bounded queues, and short critical
sections. Board support selects the hardware details and enabled features.

Display support has two hardware models:

- Standalone display firmware: the display board has its own MCU and firmware
  image, for example the BTT TFT35 E3. Build it with `BUILD_TARGET=display` and
  `DISPLAY_BOARD_NAME=<board>`.
- Mainboard-attached display module: the display is a panel wired to a
  mainboard, for example a BTT Mini12864 on an SKR expansion header. Build it
  into the mainboard image with `BUILD_TARGET=mainboard` and
  `MAINBOARD_DISPLAY_NAME=<module>`.

Board-role hardware abstraction layer (HAL) methods are documented in
[`HAL_API.md`](HAL_API.md).

Scheduler task setup, priority guidance, preemption limits, and current
scheduler limitations are documented in [`SCHEDULER.md`](SCHEDULER.md).

Display menu structure, large TFT and compact 128x64 layouts, operator screens,
safety cues, coordinate views, and settings screens are documented in
[`DISPLAY_MENU.md`](DISPLAY_MENU.md).

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
`DISPLAY_BOARD_NAME=btt_tft35_e3`. `MAINBOARD_DISPLAY_NAME=none` means the
mainboard image has no attached display module. Override names on the command
line when adding another board or display module:

```sh
make -C firmware mainboard MAIN_BOARD_NAME=btt_skr_mini_e3_v3
make -C firmware display DISPLAY_BOARD_NAME=btt_tft35_e3
make -C firmware mainboard \
  MAIN_BOARD_NAME=btt_skr_mini_e3_v3 \
  MAINBOARD_DISPLAY_NAME=btt_mini12864
```

BTT bootloader-compatible boards also emit an SD-card update filename in the
same build directory. For the default SKR Mini E3 V3 mainboard, copy
`firmware/build/mainboard/btt_skr_mini_e3_v3/firmware.bin` to the mainboard SD
card root. For the default GD32 TFT35 E3 display, copy
`firmware/build/display/btt_tft35_e3/BIGTREE_GD_TFT35_V3.0_E3.27.x.bin` to the
display SD card root. These images are linked above the factory bootloader
regions so normal updates do not replace BTT's bootloader.

If a board has previously been direct-flashed and no longer has a working BTT
bootloader at the start of flash, either keep using a direct-SWD recovery image
or restore the copied BTT bootloader. The direct-SWD image links the app at
`0x08000000`, overwrites the bootloader region, and flashes under reset:

```sh
make -C firmware/boards/display/btt_tft35_e3/build flash-direct-reset
```

To restore BTT SD-card update behavior, burn the real BTT GD TFT35 bootloader
region included in this project, then burn this project's display firmware at
the BTT application offset. The restore target uses only the first 12 KiB of
BTT's full 256 KiB flash image so it does not write into the app region:

```sh
make -C firmware/boards/display/btt_tft35_e3/build flash-btt-bootloader
make -C firmware/boards/display/btt_tft35_e3/build flash-reset
```

Check the selected build settings with:

```sh
make -C firmware print-config
make -C firmware print-config BUILD_TARGET=display
make -C firmware print-config BUILD_TARGET=display FLASH_LAYOUT=direct
make -C firmware print-config \
  BUILD_TARGET=mainboard \
  MAINBOARD_DISPLAY_NAME=btt_mini12864
```

## Current Firmware Targets

- `firmware/Makefile` - shared firmware build entry point. It selects the main
  controller with `MAIN_BOARD_NAME` and the display controller with
  `DISPLAY_BOARD_NAME`. Use `make -C firmware mainboard` for the default
  mainboard build, `make -C firmware display` for the default display build, or
  override names on the command line. Mainboard-attached display panels are
  selected with `MAINBOARD_DISPLAY_NAME`; when set to a value other than
  `none`, the output path and binary name include `-with-<module>` so plain and
  display-equipped mainboard images do not overwrite each other. Shared runtime
  sources under `firmware/core/runtime/` are compiled into each board-role
  build.
- `firmware/boards/display/btt_tft35_e3/` - initial GD32F205 bring-up for the
  BTT TFT35 E3 display. It includes startup code, linker script, GDB script,
  and Makefile targets for ST-Link/OpenOCD flash and debug. The board SysTick
  provides the monotonic millisecond clock used by the shared runtime
  scheduler. LCD, touch, encoder, backlight, buzzer, and knob LED hardware
  details stay in the board code. The image is linked at the BTT display app
  offset `0x08003000` by default, or at `0x08000000` with
  `FLASH_LAYOUT=direct` for SWD recovery without a bootloader. The copied BTT
  GD TFT35 bootloader is stored under the board's `bootloader/` directory and
  can be restored with the board Makefile. See [`HAL_API.md`](HAL_API.md) for
  the display HAL methods that the shared display entry point calls.
- `firmware/boards/mainboard/btt_skr_mini_e3_v3/` - initial STM32G0B1RET6
  bring-up skeleton for the BTT SKR Mini E3 V3 mainboard. It includes startup
  code, linker script, GDB script, and Makefile targets for ST-Link/OpenOCD
  flash and debug. The image is linked at the BTT mainboard app offset
  `0x08002000`.
- `firmware/boards/display_module/btt_mini12864/` - placeholder for a BTT
  Mini12864-style compact display connected to a mainboard expansion header.
  It is compiled into a mainboard firmware image with
  `MAINBOARD_DISPLAY_NAME=btt_mini12864`; hardware pins and LCD controller
  behavior still need bench verification.
