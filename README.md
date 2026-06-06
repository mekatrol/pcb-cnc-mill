# pcb-cnc-mill
Firmware for a PCB milling CNC.

## Project Direction

This firmware is for hobby PCB mill control. It takes ideas from Klipper,
Marlin, and GRBL: host command handling, G-code parsing, planned motion,
precise step generation, spindle control, and clear machine safety states.

## Codex Project Notes

Codex guidance lives in `.codex/`:

- `.codex/todo.md` - implementation backlog.
- `.codex/design.md` - firmware design and approach.
- `.codex/standards.md` - rules Codex must follow when making changes.

Important rule: every project change should keep this README current.
