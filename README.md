# pcb-cnc-mill
Firmware for a PCB milling CNC.

## Project Direction

This firmware is for hobby PCB mill control. It takes ideas from Klipper,
Marlin, and GRBL: host command handling, G-code parsing, planned motion,
precise step generation, spindle control, and clear machine safety states.

## Firmware Layout

Firmware is grouped by function first, then by hardware brand and model where
needed. This keeps shared control code separate from board pin maps and vendor
details.

- `firmware/core/` - hardware-neutral parser, command, state, safety, planner,
  and step scheduling code. G-code parsing, command dispatch, machine state,
  safety state, motion planning, and step scheduling belong here.
- `firmware/hal/` - low-level MCU wrappers for GPIO, timers, PWM, serial, flash,
  ADC, and interrupts. This layer keeps core firmware independent from one board
  or vendor SDK.
- `firmware/boards/` - board-specific support grouped by hardware function
  first, then by vendor and model at the leaf. Pin maps, MCU quirks, timer
  choices, ports, and board feature flags belong here.
- `firmware/boards/mainboard/` - main controller board definitions, such as
  `btt_skr_mini_e3_v3/`, `fysetc_spider_king_10/`, and `duet_3_mini_5_plus/`.
- `firmware/boards/display/` - display and pendant board definitions, such as
  `btt_tft25/`.
- `firmware/boards/toolhead/` - remote toolhead and IO board definitions, such
  as `btt_ebb36/` and `btt_ebb42/`.
- `firmware/configs/machines/` - per-machine config for limits, travel, units,
  selected boards, spindle settings, and safe defaults. Machine limits should
  live in config, not hidden source constants.

Rule of thumb: core code should not know a board brand exists. Board folders
hold pins, ports, MCU quirks, and feature flags. Machine config ties selected
hardware to one CNC build.

## Codex Project Notes

Codex guidance lives in `.codex/`:

- `.codex/todo.md` - implementation backlog.
- `.codex/design.md` - firmware design and approach.
- `.codex/standards.md` - rules Codex must follow when making changes.

Important rule: every project change should keep this README current.
