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
- `firmware/core/display/display_render.*`, `display_surface.*`, and
  `display_strings.*` - shared display presentation code. Layout, text drawing,
  and common UI strings live here so boards with the same display profile can
  reuse the same screen rendering while keeping constant strings in read-only
  data sections where the target processor/linker support that separation.
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

For the BTT SKR Mini E3 V3 mainboard, startup relocates the interrupt vector
table for the BTT bootloader app offset, then board startup initializes clocks,
GPIO, timers, safety inputs, and the STM32G0 USB device peripheral before the
shared USB device state machine starts. The main loop gates the USB
millisecond tick from the board SysTick counter so suspend and disconnect
detection use host-frame-scale timing without blocking other background work.
The same board keeps diagnostics and the TFT35 E3 link on separate
interrupt-buffered serial ports: USART1 on EXP1 for diagnostics and USART2 on
the TFT header for CRC-protected display heartbeat frames and future display
protocol traffic.

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
make -C firmware toolhead
make -C firmware flash BUILD_TARGET=mainboard
make -C firmware flash BUILD_TARGET=display
make -C firmware flash BUILD_TARGET=toolhead
```

Run these commands from inside `firmware/`:

```sh
make mainboard
make display
make toolhead
make flash BUILD_TARGET=mainboard
make flash BUILD_TARGET=display
make flash BUILD_TARGET=toolhead
```

The default boards are `MAIN_BOARD_NAME=btt_skr_mini_e3_v3`,
`DISPLAY_BOARD_NAME=btt_tft35_e3`, and
`TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1`. `MAINBOARD_DISPLAY_NAME=none` means
the mainboard image has no attached display module. Override names on the
command line when adding another board or display module:

```sh
make -C firmware mainboard MAIN_BOARD_NAME=btt_skr_mini_e3_v3
make -C firmware display DISPLAY_BOARD_NAME=btt_tft35_e3
make -C firmware toolhead TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1
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

When more than one ST-Link is attached, pass the desired probe serial with
`STLINK_SERIAL=<serial>`. With the probes currently used on this bench, the
GD32 TFT35 E3 display reports as `STM32F1xx_CL` on serial
`37FF71064E57343655651343`, and the SKR Mini E3 V3 reports as
`STM32G0Bx_G0Cx` on serial `37FF71064E573436EDA61343`:

```sh
make -C firmware/boards/display/btt_tft35_e3/build flash-reset \
  STLINK_SERIAL=37FF71064E57343655651343
make -C firmware/boards/mainboard/btt_skr_mini_e3_v3/build flash-reset \
  STLINK_SERIAL=37FF71064E573436EDA61343
```

For the default GD32 TFT35 E3 display SD-card update flow, build the display
firmware, find the mounted card, copy the BTT bootloader update file to the
card root, flush writes, and unmount it before removing the card. Replace
`SDCARD` if the card is mounted with a different volume label:

```sh
make -C firmware display
cp firmware/build/display/btt_tft35_e3/BIGTREE_GD_TFT35_V3.0_E3.27.x.bin /media/$USER/SDCARD/
sync
umount /media/$USER/SDCARD
```

Insert the microSD card into the TFT35 E3 and reset or power-cycle the display
so the BTT bootloader can burn the update image.

If a board has previously been direct-flashed and no longer has a working BTT
bootloader at the start of flash, either keep using a direct-SWD recovery image
or restore the copied BTT bootloader. The direct-SWD image links the app at
`0x08000000`, overwrites the bootloader region, and flashes under reset. For
the default SKR Mini E3 V3 mainboard:

```sh
make -C firmware/boards/mainboard/btt_skr_mini_e3_v3/build flash-direct-reset
```

For the default GD32 TFT35 E3 display:

```sh
make -C firmware/boards/display/btt_tft35_e3/build flash-direct-reset
```

To restore BTT SD-card update behavior, burn the real BTT bootloader region
included in this project, then burn this project's normal firmware at the BTT
application offset. For the default SKR Mini E3 V3 mainboard, the restore
target uses an 8 KiB bootloader-region image so it does not write into the app
region:

```sh
make -C firmware/boards/mainboard/btt_skr_mini_e3_v3/build flash-btt-bootloader
make -C firmware/boards/mainboard/btt_skr_mini_e3_v3/build flash-reset
```

For the default GD32 TFT35 E3 display, the restore target uses only the first
12 KiB of BTT's full 256 KiB flash image so it does not write into the app
region:

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
  controller with `MAIN_BOARD_NAME`, the display controller with
  `DISPLAY_BOARD_NAME`, and the toolhead controller with
  `TOOLHEAD_BOARD_NAME`. Use `make -C firmware mainboard`,
  `make -C firmware display`, or `make -C firmware toolhead` for the default
  role builds. Mainboard-attached display panels are
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
  details stay in the board code; common screen rendering and display strings
  are supplied by `firmware/core/display/`. The image is linked at the BTT
  display app offset `0x08003000` by default, or at `0x08000000` with
  `FLASH_LAYOUT=direct` for SWD recovery without a bootloader. The copied BTT
  GD TFT35 bootloader is stored under the board's `bootloader/` directory and
  can be restored with the board Makefile. See [`HAL_API.md`](HAL_API.md) for
  the display HAL methods that the shared display entry point calls.
- `firmware/boards/mainboard/btt_skr_mini_e3_v3/` - initial STM32G0B1RET6
  bring-up skeleton for the BTT SKR Mini E3 V3 mainboard. It includes startup
  code, linker script, GDB script, and Makefile targets for ST-Link/OpenOCD
  flash and debug. The image is linked at the BTT mainboard app offset
  `0x08002000` by default, or at `0x08000000` with `FLASH_LAYOUT=direct` for
  SWD recovery without a bootloader. The copied BTT SKR Mini E3 V3 bootloader
  is stored under the board's `bootloader/` directory and can be restored with
  the board Makefile.
- `firmware/boards/toolhead/btt_ebb42_gen2_v1/` - initial STM32G0B1CBT6
  toolhead bring-up. It links a direct application at `0x08000000`, flashes
  through the STM32 ROM DFU bootloader with `dfu-util`, and uses TIM7 to toggle
  the onboard red `RLED` on `PA8` every 1000 ms. Factory backup and restore
  steps are in [`doco/btt_ebb42_gen2_v1/backup_restore.md`](doco/btt_ebb42_gen2_v1/backup_restore.md).
- `firmware/boards/display_module/btt_mini12864/` - placeholder for a BTT
  Mini12864-style compact display connected to a mainboard expansion header.
  It is compiled into a mainboard firmware image with
  `MAINBOARD_DISPLAY_NAME=btt_mini12864`; hardware pins and LCD controller
  behavior still need bench verification.
