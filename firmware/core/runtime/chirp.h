#ifndef PCB_CNC_MILL_RUNTIME_CHIRP_H
#define PCB_CNC_MILL_RUNTIME_CHIRP_H

#include <stdbool.h>
#include <stdint.h>

// Board support supplies a monotonic microsecond clock because each
// microcontroller family owns its own timer registers. The chirp runtime uses
// this only for low-priority feedback timing; motion-critical timing must stay
// in hardware timer code.
typedef uint32_t (*runtime_chirp_get_monotonic_microseconds_t)(void);

// Board support owns the physical buzzer or sounder pin. The chirp runtime only
// asks for a logical output level so the same state machine can be reused by
// display, mainboard, or toolhead firmware.
typedef void (*runtime_chirp_set_output_t)(bool high);

typedef struct
{
  runtime_chirp_get_monotonic_microseconds_t get_monotonic_microseconds;
  runtime_chirp_set_output_t set_output;
} runtime_chirp_driver_t;

typedef struct
{
  runtime_chirp_driver_t driver;
  bool active;
  bool request_pending;
  bool output_high;
  uint32_t requested_half_period_microseconds;
  uint32_t requested_edges;
  uint32_t active_half_period_microseconds;
  uint32_t remaining_edges;
  uint32_t next_edge_microseconds;
} runtime_chirp_t;

// Initializes caller-owned chirp state and forces the hardware output off.
// The driver callbacks must remain valid for the lifetime of the chirp object.
void runtime_chirp_initialize(runtime_chirp_t *chirp, runtime_chirp_driver_t driver);

// Queues a single chirp request and returns immediately. A newer request
// replaces any previous request that has not started yet; an active chirp is
// replaced the next time runtime_chirp_service() runs.
void runtime_chirp_request(runtime_chirp_t *chirp, uint32_t frequency_hz, uint32_t duration_milliseconds);

// Runs one bounded low-priority service step. Call this from a cooperative
// scheduler task, not from interrupt context.
void runtime_chirp_service(runtime_chirp_t *chirp);

// Returns true while a queued or active chirp still needs scheduler service.
bool runtime_chirp_has_work(const runtime_chirp_t *chirp);

#endif // PCB_CNC_MILL_RUNTIME_CHIRP_H
