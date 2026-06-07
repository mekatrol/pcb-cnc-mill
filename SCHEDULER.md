# Scheduler

This firmware uses a cooperative runtime scheduler for normal firmware work.
Hardware timers and interrupts still own precise or urgent timing. Scheduler
tasks should perform bounded service steps, update state, and return to the
main loop.

## Adding Scheduler Tasks

The runtime scheduler is cooperative, not preemptive. A task runs only when the
main loop calls `runtime_scheduler_run_ready_tasks_once()`, and once a callback
starts it keeps the CPU until it returns. Scheduler priority controls dispatch
order for ready tasks; it does not interrupt a running task.

Use the scheduler for normal firmware service work:

- polling input state
- debouncing buttons, touch, probe, and limit inputs
- refilling queues
- running communication receive or transmit state machines
- advancing display refresh work
- advancing low-priority feedback such as chirps and LEDs
- low-rate status, diagnostics, and housekeeping

Do not use scheduler callbacks for precise step pulse timing, hard emergency
stop edges, tight serial bit timing, or long blocking transfers. Put precise or
urgent hardware timing in timer or interrupt code, then hand only small events
or state flags back to scheduled work.

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
  when several tasks are ready.
- `.enabled` can keep a task in the table while disabling it for a board variant
  or unfinished feature.
- `.next_run_milliseconds` is owned by the scheduler after initialization.
- `.callback` must not block, sleep, busy-wait for a device, or call another
  task directly.

Suggested priority bands:

- `0-9`: emergency and fault follow-up, alarm state, watchdog fault handling.
- `10-29`: motion service and time-sensitive input polling or debounce.
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

Interrupt priority and scheduler priority are separate concerns. Interrupts can
preempt scheduled tasks, so interrupt service routines must be short: acknowledge
the hardware, copy a byte or timestamp, set a flag, and return. Do not move
normal task work into a high-priority interrupt just to make it run sooner. If
work can tolerate scheduler latency, keep it scheduled. If it cannot tolerate
scheduler latency, it should be a hardware timer or interrupt path with a small
scheduled follow-up.

## Current Limitations

- Tasks are static caller-owned table entries; there is no dynamic task
  allocation.
- A callback has no payload argument. Put feature state in the owning module.
- A running callback is not preempted by another scheduler task.
- Ready tasks are scanned by numeric priority on each scheduler pass.
- Each ready task can run once per pass after its `next_run_milliseconds`
  arrives.
- The scheduler does not enforce a time budget, deadline, watchdog policy, or
  starvation protection yet.
- Priority work queues and delayed one-shot work are design goals, but only
  direct periodic task dispatch exists today.
