# Scheduler

This firmware uses a priority-based runtime scheduler. High-priority tasks can
run preemptively from a board timer or interrupt hook, while lower-priority work
continues to run from the main loop. Hardware timers still own exact step pulse
edges and other waveform timing. Scheduler tasks should perform bounded service
steps, update state, and return.

## Adding Scheduler Tasks

The runtime scheduler uses fixed numeric priorities. Lower numbers are more
urgent. A board may set `.preemptive_dispatch_enabled = true` and choose a
`.preemptive_priority_ceiling`; tasks at or below that ceiling are run by
`runtime_scheduler_run_preemptive_ready_tasks_once()` from a timer or interrupt
context. Those tasks can interrupt lower-priority scheduler callbacks, but they
cannot re-enter themselves and cannot interrupt equal or higher-priority work.

Normal tasks run when the main loop calls
`runtime_scheduler_run_ready_tasks_once()`. When preemptive dispatch is enabled,
the main-loop dispatcher skips preemptive-priority tasks and leaves them for the
board timer or interrupt hook.

Use the scheduler for normal firmware service work:

- polling input state
- debouncing buttons, touch, probe, and limit inputs
- refilling queues
- running communication receive or transmit state machines
- advancing display refresh work
- advancing low-priority feedback such as chirps and LEDs
- low-rate status, diagnostics, and housekeeping

Use preemptive-priority scheduler tasks for urgent bounded service work such as
planner queue refill, step generator starvation checks, feed hold or resume
follow-up, and fast safety state propagation. Do not use scheduler callbacks
for precise step pulse timing, hard emergency stop edges, tight serial bit
timing, or long blocking transfers. Put exact hardware timing in timer or
interrupt code, then hand only small events or state flags back to scheduled
work.

To add a periodic task:

1. Add a short `static` callback near the role entry point or feature owner.
   The callback takes no arguments and returns `void`.
2. Add a `runtime_scheduler_task_t` record to the role task table.
3. Give the task a clear `.name`, a `.period_milliseconds`, a `.priority`, an
   `.enabled` flag, `.next_run_milliseconds = 0u`, and the `.callback`.
4. Make sure the role's `runtime_scheduler_t` points at the task table and has
   a board-provided monotonic millisecond clock.
5. Initialize the scheduler after clocks and the monotonic tick are running.
6. Keep the callback bounded. It should do one small service step, update state,
   and return.

Example:

```c
static void display_buzzer_service_task(void)
{
  display_run_buzzer_tasks();
}

static runtime_scheduler_task_t display_tasks[] = {
  {
    .name = "display-buzzer-service",
    .period_milliseconds = 0u,
    .priority = 20u,
    .enabled = true,
    .next_run_milliseconds = 0u,
    .callback = display_buzzer_service_task,
  },
};
```

Task fields:

- `.name` is for debug output and future watchdog reports.
- `.period_milliseconds` is the minimum time between task starts. Use `0u` only
  for a task that is cheap, bounded, and should be checked every scheduler pass
  while the main loop is awake.
- `.priority` is fixed dispatch order. Lower numbers run before higher numbers
  when several tasks are ready. If the scheduler's preemptive dispatch is
  enabled, priorities at or below `.preemptive_priority_ceiling` run from the
  board timer or interrupt hook.
- `.enabled` can keep a task in the table while disabling it for a board variant
  or unfinished feature.
- `.next_run_milliseconds` is owned by the scheduler after initialization.
- `.running` is owned by the scheduler after initialization. Initialize it to
  `false` in static task records or leave it zero-initialized.
- `.callback` must not block, sleep, busy-wait for a device, or call another
  task directly.

Scheduler instance fields:

- `.preemptive_dispatch_enabled` enables interrupt-level dispatch for urgent
  tasks.
- `.preemptive_priority_ceiling` defines the highest numeric priority that may
  run preemptively. For example, `29u` moves priorities `0-29` into the
  preemptive path.
- `.active_callback` and `.active_priority` are owned by the scheduler after
  initialization.

Suggested priority bands:

- `0-9`: preemptive emergency and fault follow-up, alarm state, watchdog fault
  handling.
- `10-29`: preemptive motion service, planner refill, step generator starvation
  checks, and time-sensitive input polling or debounce.
- `30-79`: communication receive/transmit service for USB, serial, CAN, or host
  links.
- `80-159`: display refresh, LEDs, buzzer feedback, and normal status
  indicators.
- `160-239`: storage, logging, diagnostics, configuration save, and low-rate
  status reports.
- `240-255`: idle or opportunistic background work.

These bands are guidance, not separate queues. Current small targets may use
closer values while only a few tasks exist; preserve the relative rule that
lower numbers run first and urgent work gets the lower number.

Interrupt priority and scheduler priority are related but separate concerns.
Hardware interrupt priority decides which interrupt can preempt another
interrupt. Scheduler priority decides which ready callback may run, and whether
that callback belongs to the preemptive scheduler path. Interrupt service
routines must stay short: acknowledge the hardware, copy a byte or timestamp,
set a flag, optionally run a bounded preemptive scheduler pass, and return.

## Current Limitations

- Tasks are static caller-owned table entries; there is no dynamic task
  allocation.
- A callback has no payload argument. Put feature state in the owning module.
- Only board-enabled preemptive-priority callbacks can interrupt lower-priority
  scheduler callbacks. There is no full thread stack switching or blocking wait
  primitive.
- Ready tasks are scanned by numeric priority on each scheduler pass.
- Each ready task can run once per pass after its `next_run_milliseconds`
  arrives.
- The scheduler does not enforce a time budget, deadline, watchdog policy, or
  starvation protection yet.
- Priority work queues and delayed one-shot work are design goals, but only
  direct periodic task dispatch exists today.
