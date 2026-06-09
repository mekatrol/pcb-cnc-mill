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

## Current bring-up scope

The board HAL initializes two interrupt-buffered serial links:

Status LED:

- Pin: `PD8`
- Pattern: toggled by the TIM7 interrupt every 1 second, giving a 1 second on /
  1 second off visible heartbeat

Diagnostics:

- Connector: SKR Mini E3 V3 `EXP1`
- Peripheral: `USART1`
- TX: `PA9` / `EXP1_3`
- RX: `PA10` / `EXP1_5`
- Format: `115200` baud, 8 data bits, no parity, 1 stop bit
- Bring-up output: one boot banner followed by `mainboard alive` every 5
  seconds

Standalone TFT35 E3 touch-screen mode link:

- Connector: SKR Mini E3 V3 `TFT`
- Peripheral: `USART2`
- TX: `PA2`
- RX: `PA3`
- Format: `115200` baud, 8 data bits, no parity, 1 stop bit

The HAL exposes non-blocking raw byte probes/read/write methods for the
mainboard-to-display protocol. USART2 is dedicated to TFT traffic so diagnostic
text on EXP1 cannot appear in the display stream. The current skeleton sends a
CRC-protected heartbeat frame every 500 ms when the USART2 transmit buffer has
space. The frame format is `0xA5, type, length, payload..., crc_hi, crc_lo`;
the heartbeat uses type `0x01`, length `0`, and CRC-16/CCITT-FALSE over
`type`, `length`, and any payload bytes. It does not yet implement status
reporting, G-code forwarding, protocol-level buffering, retries, or display
state synchronization.

USB CDC bring-up depends on two startup steps: startup relocates the interrupt
vector table for the BTT bootloader app offset, then the mainboard HAL enables
clocks, GPIO, timers, interrupts, limit inputs, and the STM32G0 USB device
peripheral before the shared USB device state is initialized. The main loop
gates the USB millisecond hook from the board SysTick counter so suspend and
disconnect detection do not run on a tight-loop counter and do not block other
background work.

This is still only the bring-up base. Pin mapping, stepper timers, USB CDC
behavior on hardware, heaters, fans, endstops, probe input, spindle IO, EEPROM,
and board safety behavior need board verification before use.
