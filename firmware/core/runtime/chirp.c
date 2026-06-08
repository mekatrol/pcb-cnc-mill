#include "chirp.h"

enum
{
  RUNTIME_CHIRP_MINIMUM_EDGES = 2u,
  RUNTIME_CHIRP_MAX_EDGES_PER_SERVICE = 1u,
};

static void runtime_chirp_stop(runtime_chirp_t *chirp)
{
  chirp->active = false;
  chirp->request_pending = false;
  chirp->output_high = false;
  chirp->remaining_edges = 0u;
  chirp->next_edge_microseconds = 0u;
  chirp->active_half_period_microseconds = 0u;
  chirp->requested_edges = 0u;
  chirp->requested_half_period_microseconds = 0u;
  chirp->driver.set_output(false);
}

void runtime_chirp_initialize(runtime_chirp_t *chirp, runtime_chirp_driver_t driver)
{
  chirp->driver = driver;
  runtime_chirp_stop(chirp);
}

void runtime_chirp_request(runtime_chirp_t *chirp, uint32_t frequency_hz, uint32_t duration_milliseconds)
{
  if (frequency_hz == 0u || duration_milliseconds == 0u)
  {
    return;
  }

  uint32_t half_period_microseconds = 1000000u / (frequency_hz * 2u);
  if (half_period_microseconds == 0u)
  {
    half_period_microseconds = 1u;
  }

  uint32_t edges = duration_milliseconds * frequency_hz * 2u / 1000u;
  if (edges < RUNTIME_CHIRP_MINIMUM_EDGES)
  {
    edges = RUNTIME_CHIRP_MINIMUM_EDGES;
  }

  // The caller should be able to submit feedback and continue immediately.
  // This only records the newest request; the scheduler task below owns the
  // hardware edge timing and output shutoff.
  chirp->requested_half_period_microseconds = half_period_microseconds;
  chirp->requested_edges = edges;
  chirp->request_pending = true;
}

void runtime_chirp_service(runtime_chirp_t *chirp)
{
  if (chirp->request_pending)
  {
    chirp->request_pending = false;
    chirp->active = true;
    chirp->output_high = true;
    chirp->active_half_period_microseconds = chirp->requested_half_period_microseconds;
    chirp->remaining_edges = chirp->requested_edges - 1u;
    chirp->next_edge_microseconds =
        chirp->driver.get_monotonic_microseconds() + chirp->active_half_period_microseconds;
    chirp->driver.set_output(true);
  }

  if (!chirp->active)
  {
    return;
  }

  uint32_t edges_serviced = 0u;
  while (edges_serviced < RUNTIME_CHIRP_MAX_EDGES_PER_SERVICE &&
         (int32_t)(chirp->driver.get_monotonic_microseconds() - chirp->next_edge_microseconds) >= 0)
  {
    if (chirp->remaining_edges == 0u)
    {
      chirp->active = false;
      break;
    }

    chirp->output_high = !chirp->output_high;
    chirp->driver.set_output(chirp->output_high);
    chirp->remaining_edges--;
    chirp->next_edge_microseconds += chirp->active_half_period_microseconds;
    edges_serviced++;
  }

  if (chirp->active && chirp->remaining_edges == 0u &&
      (int32_t)(chirp->driver.get_monotonic_microseconds() - chirp->next_edge_microseconds) >= 0)
  {
    chirp->active = false;
  }

  if (!chirp->active)
  {
    runtime_chirp_stop(chirp);
  }
}

bool runtime_chirp_has_work(const runtime_chirp_t *chirp)
{
  return chirp->request_pending || chirp->active;
}
