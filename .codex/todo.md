# Todo

## Firmware Base

- Pick MCU target board and toolchain.
- Add build system for firmware image.
- Add shared bare-metal runtime used by mainboard, display, and toolhead builds.
- Add board role selection for `mainboard`, `display`, and `toolhead`.
- Add board feature flags so common code can include only the drivers each
  board needs.
- Add hardware pin map for steppers, spindle, endstops, probe, fans, and emergency stop.
- Add config file format for machine limits and board settings.
- Add serial command transport.
- Add startup self-checks.

## Shared Runtime And Scheduler

- Define common startup flow: reset handler, clock setup, board HAL init,
  driver init, scheduler init, application init, then scheduler loop.
- Add monotonic system tick for scheduler timing.
- Add cooperative task table with task name, period, priority order, enabled
  flag, and callback.
- Add scheduler loop that runs ready tasks and requires each task to return
  quickly.
- Add task watchdog policy so the watchdog is fed only when critical tasks are
  healthy.
- Add lightweight event flags for interrupt-to-task notifications.
- Add priority work item type with priority band, callback, payload pointer or
  small inline payload, `not_before` time, optional deadline, and source tag.
- Add bounded priority work queues for one-shot work submitted by drivers,
  command handlers, UI code, and interrupt follow-up code.
- Add reserved queue capacity for emergency and motion work so display,
  feedback, storage, or logging work cannot fill every slot.
- Add scheduler dispatch budget so each loop runs a bounded amount of queued
  work before returning to polling time-critical services.
- Add queue full behavior with explicit results: accepted, delayed, coalesced,
  rejected, or dropped as stale.
- Add starvation protection for normal operation while preserving emergency and
  motion-critical priority.
- Add delayed work support using `not_before` scheduler time.
- Add optional deadline checks so stale display, status, or input work can be
  dropped safely.
- Add bounded ring buffer helpers for command input, CAN messages, display
  events, and planner segments.
- Add short critical-section helpers for shared state touched by interrupts.
- Add host-build scheduler tests for task ordering, periodic timing, disabled
  tasks, and overrun detection.
- Add host-build priority queue tests for priority order, delayed dispatch,
  queue full handling, coalescing, starvation protection, and stale deadline
  drops.

## Interrupt And Timer Base

- Define common interrupt priority rules for all board roles.
- Add hardware timer abstraction for one-shot and periodic timer modes.
- Add timer compare support for precise step pulse scheduling.
- Add emergency stop interrupt path that cuts motion and spindle without waiting
  for the scheduler.
- Add limit and probe interrupt or filtered input path.
- Add bench test notes for timer jitter, interrupt latency, and emergency stop
  response time.

## Motion Control

- Add stepper driver abstraction.
- Add timer-based step pulse generator.
- Add motion planner queue.
- Add planner-to-step-generator ring buffer using the shared runtime helpers.
- Add scheduler task that refills the planner queue before it can starve.
- Add motion command work item that queues validated moves into the planner
  without generating step pulses directly.
- Add step generator starvation alarm.
- Add acceleration and junction speed handling.
- Add soft limit checks.
- Add homing sequence.
- Add probe move support for PCB height mapping.

## G-code

- Add G-code parser.
- Add command dispatcher.
- Support core CNC commands: `G0`, `G1`, `G2`, `G3`, `G4`, `G20`, `G21`, `G28`, `G38.x`, `G90`, `G91`, `G92`.
- Support core machine commands: `M3`, `M4`, `M5`, `M17`, `M18`, `M112`, `M114`, `M115`.
- Add clear error responses.
- Add command acknowledgement behavior.

## Spindle And Tooling

- Add spindle PWM control.
- Add spindle enable and direction control.
- Add tool length/probe workflow.
- Add feed hold and resume.
- Add emergency stop path that cuts motion and spindle.

## Host Link

- Define host protocol behavior.
- Add status report command.
- Add config report command.
- Add firmware version report.
- Add log levels for debug builds.
- Add non-blocking USB or serial receive task.
- Add non-blocking USB or serial transmit task.
- Add CAN receive and transmit scheduler tasks for toolheads or remote IO.
- Add communication work items for received command lines, outgoing status
  frames, CAN messages, and retries.
- Add SD card polling task for file listing and streamed job input.
- Add backpressure rules so host, SD card, and CAN traffic cannot starve motion
  queue refill.

## Display And Input

- Add shared display task model for screen refresh, touch, encoder, buttons,
  buzzer, and LEDs.
- Add display command work queue for draw text, draw region, clear region,
  status update, menu update, and progress update commands.
- Add display command coalescing so repeated redraws or status updates can merge
  before drawing.
- Add buzzer feedback work items for single chirp, repeated chirp pattern, and
  chirp stop timing.
- Add button debounce task using scheduler time.
- Add user input event queue with event type, timestamp, source, value, and
  debounce or filter state.
- Add screen refresh task that updates bounded regions instead of blocking on a
  full redraw.
- Add display command queue so mainboard communication does not block UI work.
- Add display bench test notes for touch latency, encoder response, and screen
  update time.

## Toolhead And Remote IO

- Add common toolhead application entry point using the shared runtime.
- Add CAN message routing for toolhead status, sensor readings, and output
  control.
- Add local IO task for fans, probes, sensors, and auxiliary outputs.
- Add heartbeat and timeout handling so the mainboard can fail safe if a
  toolhead stops responding.
- Add bench test notes for CAN latency and timeout behavior.

## Test And Safety

- Add unit tests for parser and planner math.
- Add simulation mode for motion planner.
- Add host simulation for scheduler load while motion queue refill is active.
- Add integration test for command input, planner queue refill, and step segment
  consumption.
- Add bench test checklist.
- Add safety interlock tests.
- Add worst-case timing checklist for USB, SD card, CAN, display, button input,
  and step generation running together.
- Add README updates for each implemented change.
