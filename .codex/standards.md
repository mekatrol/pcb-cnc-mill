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

## Firmware Rules

- Safety beats speed.
- No motion from invalid command.
- Emergency stop path must be simple and fast.
- Keep interrupt code small.
- Keep parser, planner, and hardware layers separate.
- Put board pins in one clear place.
- Put machine limits in config, not hidden constants.
- Use integer or fixed-point math for timing-critical paths when needed.
- Document units for all config and motion values.

## Test Rules

- Add or update tests when behavior changes.
- Parser and planner logic need host-runnable tests where practical.
- For hardware-only behavior, add bench test notes.
- Report tests run in final response.
