#ifndef PCB_CNC_MILL_RUNTIME_SCHEDULER_H
#define PCB_CNC_MILL_RUNTIME_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

// Scheduler task callbacks run either from the normal main-loop dispatcher or,
// for priorities at or above the configured preemptive priority ceiling, from a
// board timer or interrupt hook. A callback must do a small, bounded amount of
// work and return; it must not wait for Universal Serial Bus (USB), serial,
// Controller Area Network (CAN), storage, display transfer, input, or another
// scheduled task.
typedef void (*runtime_scheduler_task_callback_t)(void);

// Board support provides the monotonic millisecond clock because each
// microcontroller family owns its own timer registers and interrupt setup. The
// runtime scheduler only needs a steadily increasing time value for normal
// non-critical task dispatch.
typedef uint32_t (*runtime_scheduler_get_monotonic_milliseconds_t)(void);

// Describes one recurring scheduler task.
//
// This type is shared by mainboard, display, and toolhead firmware. Keep it
// hardware-neutral: board-specific pins, timer registers, bus handles, and
// device state belong in the callback owner, not in the scheduler task record.
typedef struct
{
  // Human-readable task name for debug output and future watchdog reports.
  const char *name;

  // Minimum time between task starts, in milliseconds. This scheduler is for
  // firmware service work such as polling, queue refills, input debounce,
  // display refresh, and communication housekeeping; precise step pulse edges
  // must stay in hardware timer compare code.
  uint32_t period_milliseconds;

  // Fixed priority order for ready tasks. Lower numeric values mean higher
  // urgency. When preemptive dispatch is enabled for a scheduler instance,
  // tasks at or below the scheduler's preemptive_priority_ceiling can interrupt
  // lower-priority main-loop work through the board's timer or interrupt hook.
  uint8_t priority;

  // Allows a role-specific task table to keep a task record present while
  // disabling it for a board variant or feature flag.
  bool enabled;

  // Next monotonic millisecond timestamp at which this task may run. The
  // scheduler updates this after dispatch using wrap-safe unsigned arithmetic.
  uint32_t next_run_milliseconds;

  // True while this task callback is active. This prevents a periodic task from
  // re-entering itself if a preemptive scheduler tick arrives before the prior
  // invocation has returned.
  volatile bool running;

  // Function that performs one short service step for the task.
  runtime_scheduler_task_callback_t callback;
} runtime_scheduler_task_t;

// Scheduler instance state. Each firmware role can own one scheduler instance
// with its own task table and board-provided clock source while reusing the
// same runtime dispatch code.
typedef struct
{
  // Caller-owned array of task records. The scheduler mutates
  // next_run_milliseconds but does not allocate or copy the table.
  runtime_scheduler_task_t *tasks;

  // Number of records in the task table.
  uint32_t task_count;

  // Board or test supplied monotonic millisecond clock.
  runtime_scheduler_get_monotonic_milliseconds_t get_monotonic_milliseconds;

  // Enables priority-based preemptive dispatch for urgent service tasks. Board
  // support must call runtime_scheduler_run_preemptive_ready_tasks_once() from a
  // timer or interrupt context when this is true.
  bool preemptive_dispatch_enabled;

  // Highest numeric priority that may run from the preemptive dispatch path.
  // Lower numbers are more urgent, so a ceiling of 9 allows priorities 0..9 to
  // preempt normal main-loop work. Ignored when preemptive dispatch is disabled.
  uint8_t preemptive_priority_ceiling;

  // Tracks whether any scheduler callback is currently active. Interrupt-level
  // preemptive dispatch uses this with active_priority to permit only a higher
  // priority callback to interrupt the current callback.
  volatile bool active_callback;

  // Priority of the currently active callback. Lower numeric values are more
  // urgent. This field is meaningful only when active_callback is true.
  volatile uint8_t active_priority;
} runtime_scheduler_t;

// Initializes scheduler timing by making every enabled task ready to run on the
// next scheduler pass. Call this after the board clock and monotonic tick have
// been initialized.
void runtime_scheduler_initialize(runtime_scheduler_t *scheduler);

// Runs each normal task that is ready at the current monotonic time, ordered by
// fixed priority. When preemptive dispatch is enabled, this main-loop dispatcher
// leaves preemptive-priority tasks for the board timer or interrupt hook.
void runtime_scheduler_run_ready_tasks_once(runtime_scheduler_t *scheduler);

// Runs ready tasks whose priority is at or above the configured preemptive
// ceiling. Call this only from a board timer or interrupt hook. It will not
// interrupt an equal or higher-priority callback that is already active.
void runtime_scheduler_run_preemptive_ready_tasks_once(runtime_scheduler_t *scheduler);

#endif // PCB_CNC_MILL_RUNTIME_SCHEDULER_H
