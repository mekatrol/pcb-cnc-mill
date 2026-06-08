# BTT TFT35 E3

Initial board target for the BIGTREETECH TFT35 E3 display controller.

Known controller variant on this board:

- Board: BIGTREETECH TFT35 E3 V3.0.1, GD32 variant
- MCU: GigaDevice GD32F205
- Core: ARM Cortex-M3
- Flash: 256 KiB
- RAM: linked as 64 KiB for conservative direct-SWD bring-up; some BTT GD32
  firmware configs use 128 KiB, but this ST-Link setup reports 64 KiB
- Debug and flash: ST-Link over SWD
- Firmware app offset: `0x08003000`, leaving the factory BTT TFT35 SD-card
  bootloader at the start of flash

Build from this folder:

```sh
cd firmware/boards/display/btt_tft35_e3/build
make
```

The build emits both the long project binary name and a BTT TFT35 bootloader
update image:

```text
firmware/build/display/btt_tft35_e3/BIGTREE_GD_TFT35_V3.0_E3.27.x.bin
```

To update through the factory BTT display bootloader, format a microSD card as
FAT32, copy `BIGTREE_GD_TFT35_V3.0_E3.27.x.bin` to the card root, insert it
into the TFT35 E3, and reset or power-cycle the display.

Flash with ST-Link only when doing bench bring-up or recovery:

```sh
make flash
```

When more than one ST-Link is attached, select the display probe explicitly:

```sh
make flash-reset STLINK_SERIAL=37FF71064E57343655651343
```

On the current bench setup this display probe reports as `STM32F1xx_CL` because
the GD32F205 identifies through ST-Link tooling like an STM32F1 connectivity
line device.

The GD32F205 can identify to ST-Link tooling as `STM32F1xx_CL`. `make flash`
uses `st-flash` instead of OpenOCD because this OpenOCD install can halt the
core with the GD32 TAP ID, but its STM32F2 flash driver refuses the GD32 device
ID. `make flash` writes the application at `0x08003000`. It does not
intentionally erase or program the BTT display bootloader region at
`0x08000000` through `0x08002fff`. The app startup stops any inherited SysTick
configuration and rebuilds the system clock from IRC8M before enabling the
project tick, so direct-SWD and BTT-bootloader launches use the same runtime
clock setup.

If the display was previously direct-flashed at `0x08000000`, the BTT
bootloader may no longer be present to jump to the app at `0x08003000`.
For app-only bench recovery, use the direct-SWD recovery layout:

```sh
make flash-direct-reset
```

This builds `firmware/build/display/btt_tft35_e3-direct/` and writes the app at
`0x08000000`. It overwrites the BTT bootloader region, so use it for bench
recovery, not for SD-card updates.

To restore BTT SD-card update behavior, first burn the copied BTT GD TFT35
bootloader region at `0x08000000`. The upstream BTT file is a full 256 KiB
flash image, so this target intentionally flashes only the first 12 KiB
bootloader region and does not overwrite the app at `0x08003000`:

```sh
make flash-btt-bootloader
```

Then burn this project's normal display firmware at the BTT app offset
`0x08003000`:

```sh
make flash-reset
```

After the bootloader is restored, SD-card updates can also use
`firmware/build/display/btt_tft35_e3/BIGTREE_GD_TFT35_V3.0_E3.27.x.bin`.

Start an OpenOCD debug session:

```sh
make debug
```

Then connect GDB with `debug.gdb`.

The local OpenOCD target script is still `stm32f2x.cfg` because this OpenOCD
install does not ship a `gd32f2x.cfg`. The Makefile passes `CPUTAPID
0x1ba01477`, which is the Cortex-M debug TAP ID reported by GD32F205 parts.
Without that override OpenOCD rejects the target while expecting the STM32F2
TAP ID `0x2ba01477`.

This is only the bring-up base. Pin mapping, LCD bus, touch controller, SD card,
encoder, buzzer, serial ports, and bootloader/update behavior still need board
verification before they are used.

## Current bring-up scope

The first display bring-up code follows BIGTREETECH's GD32 TFT35 E3 V3.0
variant path: `BIGTREE_GD_TFT35_E3_V3_0` includes
`pin_GD_TFT35_E3_V3_0.h`, which inherits `pin_GD_TFT35_V3_0.h` and the TFT35
V3.0 pin map. This matches the V3.0.1 board in use here:

- LCD: 16-bit FSMC 8080 bus, command at `0x60FFFFFE`, data at `0x61000000`
- Backlight: `PD12`
- Knob RGB LED data: `PC7`, with 2 NeoPixels on the GD/V3.0.1 board
- Buzzer/sounder: `PD13`
- Rotary encoder: `PA8` / `PC9`
- Encoder push button: `PC8`
- Encoder enable: `PC6`
- Touch controller: XPT2046 software SPI on `PE6` CS, `PE5` SCK, `PE4`
  MISO, `PE3` MOSI, and `PC13` pen interrupt

The display bring-up runs the GD32F205 at 120 MHz from the internal IRC8M/2
PLL source. The external HXTAL pins overlap LCD data pins `PD0` and `PD1`, so
the LCD bus remaps those pins to EXMC and does not depend on HXTAL after reset.

The firmware currently initializes the GPIO/FSMC bus, probes the TFT controller
using the same ILI9488/NT35310/ST7796S ID checks used by the upstream
BIGTREETECH TFT35 V3.0 firmware, runs the matching LCD initialization sequence,
hands a 480x320 drawing surface to the shared core display renderer, polls the
encoder and button, polls the touch controller, and queues a low-priority
scheduler chirp of the sounder once per debounced encoder or touch button
press. The first rendered screen is the shared large-TFT home screen with
offline-safe placeholder machine state until the mainboard status link is
implemented. Screen text and layout now live under `firmware/core/display/`;
this board folder owns only the physical LCD bus, controller setup, touch,
encoder, backlight, buzzer, and knob LED behavior. The LCD backlight and knob
RGB LEDs turn on after reset; during
normal operation the knob RGB LEDs cycle through a rainbow. The backlight and
knob LEDs turn off after 30 seconds without a touch, encoder rotation, or
encoder button press; any of those events turns them back on and restarts the
idle timer. If no known LCD controller ID can be read, initialization stops, the
knob RGB LED flashes red, and the buzzer emits a repeating error pulse. SD/USB
media and serial behavior are still intentionally out of scope.
