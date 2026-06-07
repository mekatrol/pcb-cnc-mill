#include "display_hal.h"
#include "runtime/scheduler.h"

static void display_background_service_task(void)
{
  display_run_background_tasks();
}

static runtime_scheduler_task_t display_tasks[] = {
  {
    .name = "display-background-service",
    .period_milliseconds = 1u,
    .priority = 10u,
    .enabled = true,
    .next_run_milliseconds = 0u,
    .callback = display_background_service_task,
  },
};

static runtime_scheduler_t display_scheduler = {
  .tasks = display_tasks,
  .task_count = sizeof(display_tasks) / sizeof(display_tasks[0]),
  .get_monotonic_milliseconds = display_get_monotonic_milliseconds,
};

int main(void)
{
  // Keep the shared display entry point hardware-neutral. The selected display
  // board owns clocks, pins, LCD bus setup, touch inputs, and local UI polling.
  display_initialize_hardware();
  runtime_scheduler_initialize(&display_scheduler);

  while (1)
  {
    runtime_scheduler_run_ready_tasks_once(&display_scheduler);
    display_wait_for_scheduler_tick();
  }
}
