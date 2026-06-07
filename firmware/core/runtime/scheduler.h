#ifndef PCB_CNC_MILL_RUNTIME_SCHEDULER_H
#define PCB_CNC_MILL_RUNTIME_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

// Scheduler task callbacks run from the cooperative main-loop context. A
// callback must do a small, bounded amount of work and return; it must not wait
// for Universal Serial Bus (USB), serial, Controller Area Network (CAN),
// storage, display transfer, input, or another scheduled task.
typedef void (*runtime_scheduler_task_callback_t)(void);

// Board support provides the monotonic millisecond clock because each
// microcontroller family owns its own timer registers and interrupt setup. The
// runtime scheduler only needs a steadily increasing time value for normal
// non-critical task dispatch.
typedef uint32_t (*runtime_scheduler_get_monotonic_milliseconds_t)(void);

// Describes one recurring cooperative task.
//
// This type is shared by mainboard, display, and toolhead firmware. Keep it
// hardware-neutral: board-specific pins, timer registers, bus handles, and
// device state belong in the callback owner, not in the scheduler task record.
typedef struct
{
  // Human-readable task name for debug output and future watchdog reports.
  const char *name;

  // Minimum time between task starts, in milliseconds. This scheduler is for
  // normal firmware service work such as polling, queue refills, input debounce,
  // display refresh, and communication housekeeping; precise step pulses must
  // stay in hardware timer code.
  uint32_t period_milliseconds;

  // Fixed priority order for ready tasks. Lower numeric values run before
  // higher numeric values when several tasks are ready in the same pass.
  uint8_t priority;

  // Allows a role-specific task table to keep a task record present while
  // disabling it for a board variant or feature flag.
  bool enabled;

  // Next monotonic millisecond timestamp at which this task may run. The
  // scheduler updates this after dispatch using wrap-safe unsigned arithmetic.
  uint32_t next_run_milliseconds;

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
} runtime_scheduler_t;

// Initializes scheduler timing by making every enabled task ready to run on the
// next scheduler pass. Call this after the board clock and monotonic tick have
// been initialized.
void runtime_scheduler_initialize(runtime_scheduler_t *scheduler);

// Runs each task that is ready at the current monotonic time, ordered by fixed
// priority. This is a cooperative dispatcher: each callback must return quickly
// so the main loop can keep servicing other firmware work.
void runtime_scheduler_run_ready_tasks_once(runtime_scheduler_t *scheduler);

#endif // PCB_CNC_MILL_RUNTIME_SCHEDULER_H
