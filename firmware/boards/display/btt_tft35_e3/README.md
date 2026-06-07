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

Build from this folder:

```sh
cd firmware/boards/display/btt_tft35_e3/build
make
```

Flash with ST-Link and OpenOCD:

```sh
make flash
```

The GD32F205 can identify to ST-Link tooling as `STM32F1xx_CL`. `make flash`
uses `st-flash` instead of OpenOCD because this OpenOCD install can halt the
core with the GD32 TAP ID, but its STM32F2 flash driver refuses the GD32 device
ID. This bring-up image is linked and written at `0x08000000` for direct SWD
boot. If the factory BTT bootloader is restored later, the app can be moved back
to BTT's GD32 TFT35 application offset at `0x08003000`.

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

The firmware currently initializes the GPIO/FSMC bus, probes the TFT controller
using the same ILI9488/NT35310/ST7796S ID checks used by the upstream
BIGTREETECH TFT35 V3.0 firmware, runs the matching LCD initialization sequence,
fills the panel with a test screen, polls the encoder and button, polls the
touch controller, and chirps the sounder once per debounced encoder or touch
button press. Rotating the encoder moves the top RGB color strip in either
direction. The LCD backlight and knob RGB LEDs turn on after reset; during
normal operation the knob RGB LEDs cycle through a rainbow. The backlight and
knob LEDs turn off after 30 seconds without a touch, encoder rotation, or
encoder button press; any of those events turns them back on and restarts the
idle timer. If no known LCD controller ID can be read, initialization stops, the
knob RGB LED flashes red, and the buzzer emits a repeating error pulse. SD/USB
media, serial, and bootloader-offset builds are still intentionally out of
scope.
