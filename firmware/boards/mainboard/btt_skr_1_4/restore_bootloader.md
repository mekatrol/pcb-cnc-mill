# BTT SKR 1.4 Turbo Bootloader Restore

This procedure restores the original bootloader on a BigTreeTech SKR 1.4
Turbo mainboard with an LPC1769 microcontroller. The bootloader image is
programmed through the LPC ROM In-System Programming (ISP) serial interface,
using a CP2102 USB-to-serial adapter.

Use 3.3 V logic for all serial connections. Do not power the SKR board from
the CP2102 5 V pin.

## 1. Install Recovery Tools

Install the tools used for downloading the bootloader archive, extracting the
bootloader image, programming the LPC1769 over serial ISP, and running optional
OpenOCD diagnostics.

```bash
sudo apt update
sudo apt install -y git unzip openocd lpc21isp screen binutils
```

## 2. Clone the Bootloader Archive

Clone the archived BTT SKR 1.3, SKR 1.4, and SKR 1.4 Turbo bootloader
repository into `~/repos`, then move into the directory containing the backed
up original SKR 1.4 Turbo bootloader files.

```bash
cd ~/repos

git clone https://github.com/GadgetAngel/BTT_SKR_13_14_14T_SD-DFU-Bootloader.git

cd ~/repos/BTT_SKR_13_14_14T_SD-DFU-Bootloader/bootloader_bin/backed_up_original_bootloaders/'SKR V1.4 Turbo'
```

## 3. Extract the Bootloader-Only Image

Extract the SKR 1.4 Turbo bootloader-only ZIP file into `/tmp`. The useful
output file is expected to be:

`/tmp/skr14t-bootloader/SKRV1.4-TURBO-Bootloader-only.hex`

```bash
unzip -o "./Bootloader ONLY/SKRV1.4-TURBO-Bootloader-only.zip" -d /tmp/skr14t-bootloader

ls -l /tmp/skr14t-bootloader
```

## 4. Check the CP2102 Serial Adapter

Confirm that the USB-to-serial adapter is detected. On a typical Linux system,
the CP2102 adapter appears as `/dev/ttyUSB0`. If your adapter appears as a
different device, use that device path in the `lpc21isp` command later.

```bash
lsusb
ls /dev/ttyUSB* /dev/ttyACM* || true
```

## 5. Wire the CP2102 to the SKR Board

Connect the serial adapter to the SKR TFT serial header. The CP2102 transmit
line connects to the SKR receive line, and the CP2102 receive line connects to
the SKR transmit line.

```bash
CP2102 GND -> SKR GND
CP2102 TXD -> SKR TFT RX0
CP2102 RXD -> SKR TFT TX0
```

Use 3.3 V logic. Do not connect CP2102 5 V to the SKR board.

To enter ISP mode, hold LPC1769 pin `P2.10` low to ground during reset. Keep
`P2.10` low while `lpc21isp` starts and programs the device if needed.

## 6. Flash the Bootloader with LPC ROM ISP

Put the board into ISP mode before running the command:

1. Hold `P2.10` to ground.
2. Press and release reset.
3. Run `lpc21isp`.

The command below wipes flash and writes the bootloader-only HEX image over the
serial adapter.

```bash
lpc21isp -wipe \
  /tmp/skr14t-bootloader/SKRV1.4-TURBO-Bootloader-only.hex \
  /dev/ttyUSB0 \
  57600 \
  12000
```

An optional verify pass may falsely fail with `COMPARE_ERROR`, so treat verify
failures cautiously and cross-check with the diagnostics below when needed.

```bash
lpc21isp -wipe -verify \
  /tmp/skr14t-bootloader/SKRV1.4-TURBO-Bootloader-only.hex \
  /dev/ttyUSB0 \
  57600 \
  12000
```

## 7. Run OpenOCD Diagnostics with an ST-Link

OpenOCD diagnostics are optional, but useful when the board still appears not
to boot after programming. Start OpenOCD in one terminal.

```bash
openocd -f interface/stlink.cfg -f target/lpc17xx.cfg
```

In a second terminal, connect to the OpenOCD telnet server.

```bash
telnet localhost 4444
```

Use the following commands inside the telnet session. These commands halt the
target, inspect the LPC1769 memory mapping register, force user flash to be
mapped at address `0x00000000`, and read the vector table.

```bash
reset halt

# Check boot ROM mapping state.
mdw 0x400FC040 1

# Force user flash to be mapped at 0x00000000.
mww 0x400FC040 0x00000001

# Verify the mapping register.
mdw 0x400FC040 1

# Read the vector table.
mdw 0x00000000 16
```

A good bootloader vector table should start like this:

```text
0x00000000: 10001260 00000179 ...
```

A misleading ROM remap or stub state may look like this:

```text
0x00000000: 10001ffc 1fff0081 ...
```

## 8. Extra OpenOCD Diagnostics

These additional telnet commands are useful for checking core registers, flash
layout, flash information, vector table contents at several offsets, and basic
RAM write/read behavior.

```bash
reset halt
reg
flash banks
flash info 0
mdw 0x00000000 16
mdw 0x00001000 16
mdw 0x00002000 16
mdw 0x00003000 16

# RAM write test.
mww 0x10000000 0x12345678
mdw 0x10000000 1
```

## 9. Flash Normal Firmware from the SD Card

After the bootloader is restored, normal firmware updates should work from the
SKR SD card slot.

1. Format the SD card as FAT32.
2. Copy the firmware image to the SD card root as `firmware.bin`.
3. Insert the SD card into the SKR board.
4. Power-cycle the board.
5. Confirm that a successful flash renamed the file to `FIRMWARE.CUR`.

You can rename `FIRMWARE.CUR` back to `firmware.bin` to test the bootloader
again.
