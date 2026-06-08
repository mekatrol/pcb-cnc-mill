# BTT SKR Mini E3 V3

Initial board target for the BIGTREETECH SKR Mini E3 V3 main controller.

Known STM32 variant:

- MCU: STM32G0B1RET6
- Core: ARM Cortex-M0+
- Flash: 512 KiB
- RAM: 144 KiB
- Debug and flash: ST-Link over SWD
- Firmware app offset: `0x08002000`, leaving the factory BTT 8 KiB
  SD-card bootloader at the start of flash

Build from this folder:

```sh
cd firmware/boards/mainboard/btt_skr_mini_e3_v3/build
make
```

The build emits both the long project binary name and a BTT bootloader update
image:

```text
firmware/build/mainboard/btt_skr_mini_e3_v3/firmware.bin
```

To update through the factory BTT bootloader, format a microSD card as FAT32,
copy `firmware.bin` to the card root, insert it into the SKR Mini E3 V3, and
reset or power-cycle the board. A successful bootloader update normally renames
the file to `FIRMWARE.CUR`.

Flash with ST-Link and OpenOCD only when doing bench bring-up or recovery:

```sh
make flash
```

When more than one ST-Link is attached, select the SKR Mini E3 V3 probe
explicitly:

```sh
make flash-reset STLINK_SERIAL=37FF71064E573436EDA61343
```

On the current bench setup this mainboard probe reports as `STM32G0Bx_G0Cx`.

`make flash` writes the application at `0x08002000`. It does not intentionally
erase or program the BTT bootloader region at `0x08000000` through
`0x08001fff`.

If the mainboard was previously direct-flashed at `0x08000000`, the BTT
bootloader may no longer be present to jump to the app at `0x08002000`.
For SWD recovery without a bootloader, build and flash the direct image under
reset:

```sh
make flash-direct-reset
```

That target rebuilds with `FLASH_LAYOUT=direct`, links the image at
`0x08000000`, and overwrites the BTT bootloader region.

To restore BTT SD-card update behavior, first burn the copied BTT SKR Mini E3
V3 bootloader region at `0x08000000`. The copied file is an 8 KiB
bootloader-region image and does not overwrite the app at `0x08002000`:

```sh
make flash-btt-bootloader
```

Then burn this project's normal mainboard firmware at the BTT application
offset:

```sh
make flash-reset
```

After the bootloader is restored, SD-card updates can also use
`firmware/build/mainboard/btt_skr_mini_e3_v3/firmware.bin`.

Start an OpenOCD debug session:

```sh
make debug
```

Then connect GDB with `debug.gdb`.

This is only the bring-up base. Pin mapping, stepper timers, USB CDC, serial
ports, heaters, fans, endstops, probe input, spindle IO, EEPROM, and board
safety behavior still need board verification before use.
