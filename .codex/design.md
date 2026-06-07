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

Use a layered design with a shared bare-metal runtime. All board types use the
same runtime shape: hardware interrupts for urgent hardware events, hardware
timers for accurate timing, and a priority scheduler for firmware service work.

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

7. Priority scheduler
   - Runs small non-blocking tasks by fixed priority.
   - Allows configured high-priority tasks to run preemptively from a board
     timer or interrupt hook.
   - Owns task timing for command processing, screen refresh, button scanning,
     status reports, storage polling, and communication housekeeping.
   - Does not own step pulse timing.
   - Lets each task do bounded work, then return.
   - Dispatches queued work items by priority when they are ready to run.

## Board Runtime Model

Use bare metal plus interrupts plus a priority scheduler. Do not make an RTOS
part of the base design.

Common runtime code should be written once and reused by mainboards, displays,
and toolheads. Board-specific code only selects pins, timers, buses, clocks,
interrupt priorities, and enabled features.

- Mainboard firmware uses the common runtime for host command handling, machine
  state, motion planning, step generation, spindle control, limits, probe input,
  USB or serial transport, SD card access, and CAN communication.
- Display firmware uses the same scheduler and driver model for LCD refresh,
  touch input, encoder input, buttons, buzzer, LEDs, USB or serial transport,
  SD card access, and CAN or serial communication with the mainboard.
- Toolhead firmware uses the same scheduler and driver model for CAN
  communication, local IO, sensors, fans, heaters if fitted, probe input, and
  any local driver control.

The shared runtime should provide:

- Startup hooks for board clock, memory, and peripheral setup.
- A priority task table with task name, period, priority order, enabled flag,
  callback, and scheduler-owned running state.
- Priority work queues for one-shot commands such as chirp, draw text, parse a
  received command, enqueue a planned move, or send a status frame.
- Monotonic tick time for non-critical task scheduling.
- Interrupt registration or board-level interrupt hooks.
- Lightweight event flags or queues for communication between interrupts and
  scheduled tasks.
- Critical-section helpers for short shared-state updates.
- Watchdog feeding from a known safe scheduler point.

## Scheduler And Work Queues

Use two related mechanisms:

- Periodic scheduler tasks run service code at fixed or minimum intervals. These
  tasks poll drivers, refill queues, debounce inputs, refresh display regions,
  and process communication state machines.
- Priority work queues hold one-shot work items that can be submitted and then
  forgotten by the caller. The scheduler dispatches ready work items in priority
  order, subject to a small per-loop work budget.

Queued work items should be small commands, not long blocking operations.

Examples:

- A `chirp` work item starts or updates the buzzer state, then returns. A timer
  or later scheduler tick turns the buzzer off.
- A `draw text` work item copies text into a display command queue or marks a
  display region dirty, then returns. The display refresh task performs bounded
  drawing work over later scheduler passes.
- A `move stepper` work item means enqueue a validated motion command or planner
  segment. It must not directly bit-bang step pulses. Hardware timer code emits
  the actual step pulses from the step segment queue.
- A button, encoder, touch, probe, or limit input event carries an event type,
  timestamp, source, and value. Debounce and filtering happen before the event
  changes machine state.

Use fixed priority bands so behavior is predictable:

- Emergency and safety work: alarm state, emergency stop follow-up, hard limit
  follow-up, watchdog fault handling.
- Motion service work: planner queue refill, step generator starvation checks,
  feed hold, resume, and homing state progress.
- User input work: debounced buttons, encoder turns, touch events, feed hold,
  cancel, jog requests, and menu selection.
- Communication work: USB, serial, CAN receive and transmit handling.
- Display and feedback work: display commands, buzzer chirps, LEDs, and status
  indicators.
- Background work: SD card directory reads, logging, config save, diagnostics,
  and low-rate status reports.

Queue rules:

- Queues are bounded. Full queues must return a clear result to the caller.
- Safety and motion queues reserve space so background or display work cannot
  block urgent work.
- Work items may have a `not_before` time for delayed work, debounce expiry,
  chirp off timing, and display pacing.
- Work items may have a deadline when late work is worse than dropped work.
- Coalescing is allowed for display redraws, repeated status reports, and input
  state updates where only the latest value matters.
- No queued item may wait on USB, SD card, CAN, display transfer, button input,
  or another queued item.
- The scheduler should limit how much work it runs per pass so low priority work
  cannot make the main loop unresponsive, and board timer hooks should run only
  bounded preemptive-priority work.
- Starvation protection should allow occasional lower-priority work to run when
  high-priority work remains busy, except when the machine is in an emergency or
  motion-critical state.

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

High-level code runs through the priority scheduler. Motion-critical service
work can use preemptive-priority scheduler tasks, but step pulse generation
must use precise hardware timing and must not depend on scheduler latency.

- Interrupt or timer code stays small.
- Planner fills a queue.
- Step generator consumes the queue.
- Shared state between interrupt and main loop must be minimal and protected.
- Scheduler tasks must not block while waiting for USB, SD card, CAN, display,
  touch, button, serial, or storage work.
- Slow peripherals are split into short state-machine steps.
- Button debounce, screen updates, status reports, and storage polling use
  scheduler periods, not busy waits.
- Emergency stop and hard limit handling use direct interrupt or hardware paths
  where practical.
- Motion queue refill is scheduled often enough that the step generator does not
  starve during worst-case communication, screen, or storage activity.

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
