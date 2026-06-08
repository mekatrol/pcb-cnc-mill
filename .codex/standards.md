# Codex Standards

## Language

- Use caveman style language to save tokens.
- Prefer short words.
- Say what changed, what test ran, and what risk remains.
- Avoid long theory unless user asks.

## README Rule

- Update base `README.md` with any project change.
- If code, config, docs, build, test, or behavior changes, README must stay true.
- If README does not need much detail, add short note or link to the right doc.

## Change Rules

- Read nearby files before edit.
- Keep change small and tied to user ask.
- Follow existing style first.
- Do not rewrite unrelated files.
- Do not delete user work.
- Do not use destructive git commands unless user asks.
- Keep code split by feature or function. Entry point files such as `main.c`
  should wire startup and loops only; scheduler, timer, parser, planner,
  display, input, and driver behavior should live in their own files.
- Reserve `main.c` filenames for true entry points that define or directly
  wire the firmware main loop. Board support files should use feature names
  such as `display_hal.c`, `mainboard_hal.c`, timer, input, or driver names.
- Use common runtime names for code shared by mainboard, display, and toolhead
  builds. Do not add a board-role prefix such as `display_` to scheduler,
  timer, queue, or task types unless the type is truly display-specific.
- Keep standalone display firmware boards separate from mainboard-attached
  display modules. A module such as a Mini12864 compiled into a mainboard image
  belongs under `firmware/boards/display_module/`, not under
  `firmware/boards/display/`.

## Firmware Rules

- Safety beats speed.
- No motion from invalid command.
- Emergency stop path must be simple and fast.
- Keep interrupt code small.
- Keep parser, planner, and hardware layers separate.
- Shared runtime code belongs under `firmware/core/runtime/` unless there is a
  stronger feature-specific home.
- Add verbose comments for low-level firmware, scheduler, timer, interrupt,
  hardware register, and shared runtime code. Public structures, structure
  fields, callback types, and function headers should explain ownership, units,
  timing behavior, and hardware boundaries.
- Put board pins in one clear place.
- Put machine limits in config, not hidden constants.
- Use integer or fixed-point math for timing-critical paths when needed.
- Document units for all config and motion values.

## Test Rules

- Add or update tests when behavior changes.
- Parser and planner logic need host-runnable tests where practical.
- For hardware-only behavior, add bench test notes.
- Report tests run in final response.
