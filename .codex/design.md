# Firmware Design

## Goal

Build firmware for a hobby PCB milling CNC. Design borrows useful ideas from Klipper, Marlin, and GRBL, but keeps scope tight for milling.

Main job:

- Receive host commands.
- Parse G-code.
- Plan safe motion.
- Generate accurate step pulses.
- Control spindle and machine IO.
- Report clear status and errors.

## Approach

Use a layered design.

1. Host link
   - Handles serial or USB transport.
   - Receives command lines.
   - Sends `ok`, error, and status responses.

2. Command layer
   - Parses G-code and machine commands.
   - Converts text commands into typed internal actions.
   - Rejects bad commands before motion planning.

3. Machine state
   - Tracks position, units, distance mode, feed rate, spindle state, homing state, and alarms.
   - Owns config values such as steps/mm, max travel, max feed, acceleration, and pin map.

4. Motion planner
   - Accepts validated moves.
   - Applies limits and acceleration.
   - Queues move segments.
   - Keeps motion deterministic.

5. Step generation
   - Runs from hardware timers or equivalent precise timing.
   - Emits step and direction signals.
   - Does not parse text or make high-level decisions.

6. Hardware abstraction
   - Wraps GPIO, timers, PWM, ADC, EEPROM/flash, and interrupts.
   - Keeps board-specific code away from planner and parser code.

## Safety Model

Safety first for all control code.

- Emergency stop must stop steppers and spindle fast.
- Alarm state must block motion until reset.
- Limit and probe inputs must be debounced or filtered.
- Homing must use conservative speeds.
- Soft limits must reject moves outside configured travel.
- Parser errors must not move the machine.
- Spindle commands must have explicit state changes.

## Timing Model

High-level code may run in the main loop. Step pulse generation must use precise timing.

- Interrupt or timer code stays small.
- Planner fills a queue.
- Step generator consumes the queue.
- Shared state between interrupt and main loop must be minimal and protected.

## Configuration

Configuration should be easy to inspect and hard to misuse.

- Board pin map should live in one place.
- Machine settings should have units and bounds.
- Defaults should be safe.
- Persistent settings should have versioning.

## Testing Strategy

Prefer tests that catch firmware mistakes before hardware moves.

- Unit test parser behavior.
- Unit test unit conversion and coordinate modes.
- Unit test planner limits.
- Run planner simulation on host builds.
- Keep hardware smoke tests documented in README or dedicated docs.
