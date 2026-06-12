# BTT EBB42 Gen2 V1 Toolhead

This first STM32G0B1CBT6 bring-up image configures the onboard red `RLED` on
`PA8` and uses TIM7 to toggle it every 1000 ms. The main loop sleeps between
interrupts.

Build from the repository root:

```sh
make -C firmware toolhead TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1
```

Put the board in DFU mode, then build and flash with `dfu-util`:

```sh
make -C firmware flash \
  BUILD_TARGET=toolhead \
  TOOLHEAD_BOARD_NAME=btt_ebb42_gen2_v1
```

The image is linked and written at `0x08000000`. The ROM DFU bootloader remains
available because it is not stored in application flash. See
[`doco/btt_ebb42_gen2_v1/backup_restore.md`](../../../../doco/btt_ebb42_gen2_v1/backup_restore.md)
for the BOOT/RST sequence and factory backup restore steps.
