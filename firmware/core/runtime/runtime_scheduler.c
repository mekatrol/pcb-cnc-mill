#include "runtime_scheduler.h"

// Returns true when the scheduled timestamp has arrived or passed. The cast to
// signed arithmetic keeps the comparison correct across the 32-bit millisecond
// counter wrap as long as scheduled times are less than half the counter range
// away from the current time.
static bool runtime_scheduler_time_has_arrived(uint32_t now_milliseconds, uint32_t scheduled_milliseconds)
{
  return (int32_t)(now_milliseconds - scheduled_milliseconds) >= 0;
}

static bool runtime_scheduler_task_is_preemptive(const runtime_scheduler_t *scheduler, const runtime_scheduler_task_t *task)
{
  return scheduler->preemptive_dispatch_enabled && task->priority <= scheduler->preemptive_priority_ceiling;
}

static bool runtime_scheduler_priority_can_run_now(const runtime_scheduler_t *scheduler, uint8_t priority)
{
  return !scheduler->active_callback || priority < scheduler->active_priority;
}

static void runtime_scheduler_run_task(runtime_scheduler_t *scheduler, runtime_scheduler_task_t *task)
{
  const bool previous_active_callback = scheduler->active_callback;
  const uint8_t previous_active_priority = scheduler->active_priority;

  scheduler->active_callback = true;
  scheduler->active_priority = task->priority;
  task->running = true;

  task->callback();

  task->running = false;
  scheduler->active_priority = previous_active_priority;
  scheduler->active_callback = previous_active_callback;
}

static void runtime_scheduler_run_ready_tasks_matching_context(runtime_scheduler_t *scheduler, bool preemptive_context)
{
  const uint32_t now_milliseconds = scheduler->get_monotonic_milliseconds();

  for (uint32_t priority = 0u; priority <= 255u; priority++)
  {
    if (!runtime_scheduler_priority_can_run_now(scheduler, (uint8_t)priority))
    {
      continue;
    }

    for (uint32_t task_index = 0u; task_index < scheduler->task_count; task_index++)
    {
      runtime_scheduler_task_t *task = &scheduler->tasks[task_index];
      const bool task_is_preemptive = runtime_scheduler_task_is_preemptive(scheduler, task);

      if (task_is_preemptive != preemptive_context)
      {
        continue;
      }

      if (!task->enabled || task->running || task->priority != priority)
      {
        continue;
      }

      if (!runtime_scheduler_time_has_arrived(now_milliseconds, task->next_run_milliseconds))
      {
        continue;
      }

      task->next_run_milliseconds = now_milliseconds + task->period_milliseconds;
      runtime_scheduler_run_task(scheduler, task);
    }
  }
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
    scheduler->tasks[task_index].running = false;
  }

  scheduler->active_callback = false;
  scheduler->active_priority = 255u;
}

// Performs one normal scheduler pass. Ready tasks are searched by priority so
// service work can run before lower-priority display, feedback, storage, or
// diagnostic work. If preemptive dispatch is enabled, the highest-priority
// tasks are reserved for runtime_scheduler_run_preemptive_ready_tasks_once().
void runtime_scheduler_run_ready_tasks_once(runtime_scheduler_t *scheduler)
{
  runtime_scheduler_run_ready_tasks_matching_context(scheduler, false);
}

// Performs one preemptive scheduler pass from a board-owned timer or interrupt
// hook. Only priority-ceiling tasks run here, and a callback can interrupt only
// lower-priority scheduler work that is already active.
void runtime_scheduler_run_preemptive_ready_tasks_once(runtime_scheduler_t *scheduler)
{
  if (!scheduler->preemptive_dispatch_enabled)
  {
    return;
  }

  runtime_scheduler_run_ready_tasks_matching_context(scheduler, true);
}
