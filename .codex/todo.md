# Todo

## Firmware Base

- Pick MCU target board and toolchain.
- Add build system for firmware image.
- Add hardware pin map for steppers, spindle, endstops, probe, fans, and emergency stop.
- Add config file format for machine limits and board settings.
- Add serial command transport.
- Add startup self-checks.

## Motion Control

- Add stepper driver abstraction.
- Add timer-based step pulse generator.
- Add motion planner queue.
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

## Test And Safety

- Add unit tests for parser and planner math.
- Add simulation mode for motion planner.
- Add bench test checklist.
- Add safety interlock tests.
- Add README updates for each implemented change.
