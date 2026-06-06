# BTT SKR Mini E3 V3

Initial board target for the BIGTREETECH SKR Mini E3 V3 main controller.

Known STM32 variant:

- MCU: STM32G0B1RET6
- Core: ARM Cortex-M0+
- Flash: 512 KiB
- RAM: 144 KiB
- Debug and flash: ST-Link over SWD

Build from this folder:

```sh
cd firmware/boards/mainboard/btt_skr_mini_e3_v3/build
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

This is only the bring-up base. Pin mapping, stepper timers, USB CDC, serial
ports, heaters, fans, endstops, probe input, spindle IO, EEPROM, and board
safety behavior still need board verification before use.
