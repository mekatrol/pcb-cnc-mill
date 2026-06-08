# HAL API

This file documents the public hardware abstraction layer (HAL) methods that
board support must provide to shared core entry points. The goal is to keep
`firmware/core/` hardware-neutral: core code calls role-level methods, and the
selected board folder owns clocks, pins, registers, peripheral quirks, and local
device state.

Each board implementation should place its role header and source in:

- `firmware/boards/mainboard/<board_name>/mainboard_hal.h`
- `firmware/boards/mainboard/<board_name>/mainboard_hal.c`
- `firmware/boards/display/<board_name>/display_hal.h`
- `firmware/boards/display/<board_name>/display_hal.c`
- `firmware/boards/display_module/<module_name>/mainboard_display_module.c`
- `firmware/boards/toolhead/<board_name>/toolhead_hal.h`
- `firmware/boards/toolhead/<board_name>/toolhead_hal.c`

The shared entry point includes only the selected board's role HAL header. Do
not make core entry points include board register headers, pin maps, vendor
startup files, or board-specific driver state.

## Common Rules

HAL methods run in bare-metal firmware. They must use fixed ownership and clear
timing boundaries:

- Initialize clocks, memory, GPIO, timers, interrupts, and local devices before
  core starts the scheduler.
- Provide monotonic clocks to runtime code when a role uses the scheduler.
- Keep interrupt service routines short. Interrupts may record data, set flags,
  acknowledge hardware, feed a hardware queue, or run one bounded preemptive
  scheduler pass for urgent priorities, then return.
- Keep scheduled service methods non-blocking. They should do bounded work and
  return to the scheduler or main loop.
- Do not put parser, planner, machine state, or role-independent runtime policy
  in a board HAL.
- Do not busy-wait for USB, serial, Controller Area Network (CAN), storage,
  display transfer, input, or another scheduled task from a scheduler callback.
- Hardware timer code may own precise waveform timing. Preemptive-priority
  scheduler callbacks should own urgent setup, refill, debounce, and follow-up
  work that is safe to run from timer or interrupt context. Normal-priority
  callbacks should own less urgent service work from the main loop.

## Mainboard HAL

The mainboard HAL is the board support contract for the main controller role.
The current shared mainboard entry point is `firmware/core/mainboard/main.c`.

Required header:

```c
void mainboard_initialize_hardware(void);
void mainboard_run_background_tasks(void);
```

### `mainboard_initialize_hardware`

Purpose:

- Configure the system clock and memory timing.
- Configure GPIO modes, alternate functions, pull-ups, and safe output states.
- Initialize hardware timers for step generation, spindle control, watchdog,
  monotonic time, and other board-local timing.
- Configure the timer or interrupt hook that calls
  `runtime_scheduler_run_preemptive_ready_tasks_once()` when the mainboard task
  table enables preemptive dispatch for motion or safety service work.
- Initialize communication hardware such as USB, serial, or CAN when present.
- Initialize limit, probe, emergency stop, spindle, coolant, fan, and local IO
  pins into safe defaults.

Timing:

- Called once from the shared mainboard entry point before the main loop starts.
- May block during reset-time hardware setup, oscillator lock, or peripheral
  reset.
- Must leave unsafe outputs disabled unless later validated machine state
  enables them.

### `mainboard_run_background_tasks`

Purpose:

- Perform bounded non-motion-critical service work for the current mainboard
  skeleton.
- In future scheduler-based mainboard firmware, this should become scheduler
  dispatch or a short wrapper around scheduled task service.

Timing:

- Called repeatedly from the mainboard main loop.
- Must return quickly.
- Must not generate step pulses directly; precise step timing belongs in timer
  compare code fed by planner or step queues.

## Display HAL

The display HAL is the board support contract for display and pendant boards.
The current shared display entry point is `firmware/core/display/main.c`. Use
this contract only for display hardware that has its own microcontroller and
therefore owns a separate firmware image, reset handler, linker script, clock
tree, scheduler instance, and local display firmware main loop.

Required header:

```c
#include <stdint.h>

void display_initialize_hardware(void);
uint32_t display_get_monotonic_milliseconds(void);
void display_run_background_tasks(void);
void display_run_buzzer_tasks(void);
void display_wait_for_scheduler_tick(void);
```

### `display_initialize_hardware`

Purpose:

- Configure the display board clock tree and SysTick or equivalent monotonic
  timer.
- Configure display bus pins, backlight pins, touch pins, encoder pins, buzzer
  pins, LEDs, and any board-local communication pins.
- Initialize LCD controller, touch controller, LEDs, and local feedback devices.
- Initialize board-local runtime helpers such as the buzzer chirp driver.
- Initialize USB and SD card drivers.

Timing:

- Called once before `runtime_scheduler_initialize()`.
- May block during reset-time hardware setup and LCD controller initialization.
- Must leave the monotonic millisecond clock running before
  `display_initialize_hardware()` returns. The shared display entry point calls
  `runtime_scheduler_initialize()` after this HAL method returns, so scheduler
  initialization must be able to read a live millisecond clock immediately.

### `display_get_monotonic_milliseconds`

Purpose:

- Return a steadily increasing millisecond time base for the shared runtime
  scheduler.

Timing:

- Called by scheduler initialization and every scheduler pass.
- Must be safe to call from main-loop context and from a board timer or
  interrupt when that role enables preemptive scheduler dispatch.
- Must tolerate unsigned rollover from the maximum millisecond count back to
  `0`. Runtime code compares timestamps with wrap-safe signed subtraction, for
  example `(int32_t)(now_milliseconds - scheduled_milliseconds) >= 0`, so board
  HAL code should use the same elapsed-time pattern instead of direct `>` or
  `<` timestamp comparisons.
- Rollover is expected and should not reset scheduler state. A task scheduled
  before rollover remains due after the counter wraps as long as the requested
  delay is less than half the timestamp range.
- The rollover interval is hardware and configuration dependent: it depends on
  the timer width, whether the HAL exposes raw timer ticks or a derived
  millisecond counter, and the clock or prescaler settings used to produce the
  millisecond time base. The current BTT TFT35 E3 display runs a 120 MHz system
  clock, reloads SysTick every 120,000 core-clock ticks, and increments a
  32-bit software millisecond counter once per SysTick interrupt. That counter
  rolls over every `2^32` milliseconds, which is about 49.7 days of uptime.

### `display_run_background_tasks`

Purpose:

- Run normal display service work such as encoder polling, touch polling,
  backlight timeout checks, LED updates, and bounded display refresh work.
- Feed local input events and physical display drawing callbacks into shared
  display rendering code. Screen composition, shared text layout, menu state,
  and common display strings belong in `firmware/core/display/`, not in the
  board HAL.

Timing:

- Called from a scheduler task.
- Must do bounded work and return.
- Must not wait for another scheduled task or a long display transfer.
- Must not embed role-independent screen layouts or user-facing menu strings in
  the board HAL. Board code may expose low-level pixel, rectangle, text-bus, or
  tile transfer primitives needed by the selected display controller.

### `display_run_buzzer_tasks`

Purpose:

- Advance low-priority sounder feedback work. On the BTT TFT35 E3 display this
  delegates to the shared `runtime_chirp` state machine while the board HAL
  supplies the microsecond clock and physical buzzer output method.

Timing:

- Called from a low-priority scheduler task.
- Must return after a small amount of work.
- Must not block for the full chirp duration.

### `display_wait_for_scheduler_tick`

Purpose:

- Provide the display main loop with a board-local idle wait between scheduler
  passes.
- The wait may be shortened or skipped when board-local work needs close
  scheduler service, such as an active buzzer chirp.

Timing:

- Called once after each scheduler pass.
- Should not sleep so long that input, display, communication, or feedback
  tasks miss their expected service latency.

## Mainboard Display Module

A mainboard display module is a panel wired directly to a mainboard. It is
compiled into the mainboard firmware image with `MAINBOARD_DISPLAY_NAME=<name>`.
It does not own a reset handler, linker script, system clock, scheduler
instance, or separate firmware binary.

Shared header:

```c
#include "mainboard_display_module.h"
```

Required implementation file:

```text
firmware/boards/display_module/<module_name>/mainboard_display_module.c
```

Required methods:

```c
void mainboard_display_module_initialize_hardware(void);
void mainboard_display_module_run_background_tasks(void);
void mainboard_display_module_run_feedback_tasks(void);
```

### `mainboard_display_module_initialize_hardware`

Purpose:

- Configure only display-module-local hardware such as display bus pins,
  chip-select pins, encoder inputs, button inputs, backlight, sounder, and
  LEDs.
- Leave MCU clocks, reset-time safety output states, motion timers,
  communication peripherals, and system monotonic timers under mainboard HAL
  ownership.

Timing:

- Called once after `mainboard_initialize_hardware()` in the shared mainboard
  entry point when `MAINBOARD_DISPLAY_NAME` is not `none`.
- May perform reset-time panel setup but must not assume it owns global clock or
  interrupt policy.

### `mainboard_display_module_run_background_tasks`

Purpose:

- Run bounded input and display service work for a mainboard-attached panel:
  encoder sampling, button debounce, compact LCD redraw regions, and status
  presentation.
- In the current mainboard skeleton this is called from the main loop. When the
  mainboard entry point moves to the runtime scheduler, this work should become
  scheduler tasks in the display/input priority bands.

Timing:

- Must return quickly.
- Must not block machine control, motion service, host communication, or safety
  follow-up work.

### `mainboard_display_module_run_feedback_tasks`

Purpose:

- Run bounded local feedback work such as sounder chirps, status LEDs, or
  backlight timeout state.
- Keep feedback work separate from redraw work so future task tables can give
  each service a different period or priority.

Timing:

- Must return quickly.
- Must not busy-wait for the full sound or LED pattern duration.

## Toolhead HAL

No concrete toolhead board is implemented yet. The intended contract should
match the same role shape used by mainboard and display firmware.

Expected header:

```c
#include <stdint.h>

void toolhead_initialize_hardware(void);
uint32_t toolhead_get_monotonic_milliseconds(void);
void toolhead_run_background_tasks(void);
void toolhead_wait_for_scheduler_tick(void);
```

Expected responsibilities:

- Configure local clocks, GPIO, timers, CAN or serial links, fans, probe inputs,
  sensors, tool outputs, and any local driver hardware.
- Provide a monotonic millisecond scheduler clock.
- Run bounded scheduled service for communication, local IO, sensor polling,
  fan updates, and diagnostic/status reporting.
- Keep time-critical or safety-critical edges in hardware timer or interrupt
  paths with short scheduled follow-up work.

This section should be updated when the first toolhead entry point and board
HAL are added.

## Runtime Driver Hooks

Some shared runtime helpers use small driver callback structures instead of a
role-wide HAL function. These callbacks should still be implemented by the board
HAL because they touch hardware.

### Chirp Runtime

Header:

```c
#include "runtime/chirp.h"
```

Board-provided callbacks:

```c
uint32_t get_monotonic_microseconds(void);
void set_output(bool high);
```

Purpose:

- `get_monotonic_microseconds` returns a monotonic microsecond time base for
  low-priority feedback edge timing.
- `set_output` drives the physical buzzer or sounder output high or low.

The shared `runtime_chirp` module owns chirp request state, edge scheduling,
and output shutoff. The board HAL owns only the time source and pin write.
