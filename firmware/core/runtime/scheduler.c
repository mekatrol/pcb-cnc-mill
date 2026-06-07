#include "runtime/scheduler.h"

// Returns true when the scheduled timestamp has arrived or passed. The cast to
// signed arithmetic keeps the comparison correct across the 32-bit millisecond
// counter wrap as long as scheduled times are less than half the counter range
// away from the current time.
static bool runtime_scheduler_time_has_arrived(uint32_t now_milliseconds, uint32_t scheduled_milliseconds)
{
  return (int32_t)(now_milliseconds - scheduled_milliseconds) >= 0;
}

// Sets the first run time for every task in the caller-owned task table. The
// scheduler does not know which board role owns the tasks; it only asks the
// supplied clock function for the current monotonic scheduler time.
void runtime_scheduler_initialize(runtime_scheduler_t *scheduler)
{
  const uint32_t now_milliseconds = scheduler->get_monotonic_milliseconds();

  for (uint32_t task_index = 0u; task_index < scheduler->task_count; task_index++)
  {
    scheduler->tasks[task_index].next_run_milliseconds = now_milliseconds;
  }
}

// Performs one bounded scheduler pass. Ready tasks are searched by priority so
// urgent service work can run before lower-priority display, feedback, storage,
// or diagnostic work. Each callback is expected to perform one short state
// machine step and then return to this loop.
void runtime_scheduler_run_ready_tasks_once(runtime_scheduler_t *scheduler)
{
  const uint32_t now_milliseconds = scheduler->get_monotonic_milliseconds();

  for (uint32_t priority = 0u; priority <= 255u; priority++)
  {
    for (uint32_t task_index = 0u; task_index < scheduler->task_count; task_index++)
    {
      runtime_scheduler_task_t *task = &scheduler->tasks[task_index];

      if (!task->enabled || task->priority != priority)
      {
        continue;
      }

      if (!runtime_scheduler_time_has_arrived(now_milliseconds, task->next_run_milliseconds))
      {
        continue;
      }

      task->next_run_milliseconds = now_milliseconds + task->period_milliseconds;
      task->callback();
    }
  }
}
