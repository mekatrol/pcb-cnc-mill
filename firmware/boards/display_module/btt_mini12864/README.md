# BTT Mini12864 Display Module

Initial placeholder for a BIGTREETECH Mini12864-style compact display attached
directly to a mainboard.

This is a display module, not a standalone display firmware target. It has no
separate firmware image, reset handler, linker script, system clock, or board
scheduler. It is compiled into the selected mainboard firmware when
`MAINBOARD_DISPLAY_NAME=btt_mini12864` is used.

Example:

```sh
make -C firmware mainboard \
  MAIN_BOARD_NAME=btt_skr_mini_e3_v3 \
  MAINBOARD_DISPLAY_NAME=btt_mini12864
```

The selected mainboard owns machine state, motion, safety outputs,
communication, startup clocks, and the firmware binary. This module should own
only the panel-local hardware: display bus pins, chip-select pins, encoder and
button inputs, backlight, sounder, and LEDs.

The Mini12864 hardware mapping is not verified yet. Do not use this module for
machine control until the target mainboard connector pinout, LCD controller,
encoder wiring, contrast or backlight control, and any sounder or LED pins have
bench notes.
