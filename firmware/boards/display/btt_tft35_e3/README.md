# BTT TFT35 E3

Initial board target for the BIGTREETECH TFT35 E3 display controller.

Known STM32 variant:

- MCU: STM32F207VCT6
- Core: ARM Cortex-M3
- Flash: 256 KiB
- RAM: 128 KiB
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

Start an OpenOCD debug session:

```sh
make debug
```

Then connect GDB with `debug.gdb`.

This is only the bring-up base. Pin mapping, LCD bus, touch controller, SD card,
encoder, buzzer, serial ports, and bootloader/update behavior still need board
verification before they are used.
