# Display Menu

This document defines the display menu target for the PCB CNC mill. It is a
user-interface guide for shared display code and board hardware abstraction
layer (HAL) implementers. It does not put machine state, G-code parsing, or
motion planning in a display board HAL.

The display should behave like a small CNC control panel: show machine status,
send validated machine operations to the mainboard, and keep safety state
obvious. The menu is scoped for PCB milling and should focus on direct machine
operation rather than general-purpose printer or router workflows.

## Display Rules

- The mainboard owns machine state, motion validation, G-code execution,
  coordinates, limits, alarms, and settings persistence.
- The display owns presentation, touch or encoder input, local feedback,
  redraw pacing, and communication with the mainboard.
- A standalone display board owns its local firmware and link to the mainboard.
  A mainboard-attached display module shares the mainboard firmware image and
  talks to machine state through in-firmware interfaces instead of a board link.
- Emergency stop, hard limit, and alarm indicators must be visible from every
  screen.
- Any operation that can move an axis, start the spindle, change offsets, clear
  an alarm, erase settings, or run a file needs either a dedicated button or a
  confirmation screen.
- Jog, home, probe, and spindle commands must be rejected or disabled on the
  display when the latest mainboard state says they are unsafe.
- Units are shown beside every distance, speed, and spindle value.
- Coordinate screens must clearly label machine coordinates and active work
  coordinate system values.
- Touch and encoder builds should share the same menu model. Touch buttons are
  direct actions; encoder input moves the selection cursor and press activates
  the selected item.
- The same menu tree must support large graphical displays and compact
  monochrome displays. A board may choose a layout profile that fits its pixel
  size, font size, input device, and redraw cost.

## Layout Profiles

The display system should support at least these two layout profiles.

| Profile | Typical hardware | Minimum traits | Navigation |
| --- | --- | --- | --- |
| Large TFT | BTT TFT35 E3, 480x320 color touch display | Status bar, multi-row coordinate view, touch button grid, modal confirmations | Touch or encoder |
| Compact 128x64 | BTT MINI12864, RepRapDiscount-style monochrome LCD | Graphical 128x64 page, one selected row, short status glyphs, paged detail screens | Encoder and click button |

The large TFT examples in this document use a 480x320-style layout. Compact
128x64 displays should not try to draw those screens scaled down. They should
present the same screen state as short pages with a title row, a small status
line, three to five content rows, and a footer or selected action row.

Compact 128x64 examples use a rough 20-character documentation model inside
the border. Real firmware may use a different font, icon strip, or line count
as long as the same fields and actions remain visible.

```text
┌────────────────────┐
│12345678901234567890│
│12345678901234567890│
│12345678901234567890│
│12345678901234567890│
│12345678901234567890│
│12345678901234567890│
│12345678901234567890│
└────────────────────┘
```

Compact screen rules:

- Keep the top row for title plus machine state: for example `HOME IDLE USB✓`.
- Keep safety visible on every page with `ESTOP`, `ALARM`, `HOLD`, or `OK`.
- Show one coordinate set per page when needed. Do not mix too many long
  position values on one compact page.
- Use pages such as `1/3`, `2/3`, and `3/3` for detail screens.
- Use short labels: `MPos`, `WPos`, `Offs`, `Spn`, `Fd`, `Lim`, `Prb`.
- Use encoder rotation for row selection or value changes. Use click for
  select/apply. Use long click for back or cancel where the hardware supports
  it.
- Use confirmation pages instead of side-by-side buttons. The selected row
  should be either `Cancel` or the dangerous action.
- If a compact display has custom icons or status LEDs, they may replace text
  glyphs only when the meaning remains clear without color.

## Visual Language

Use simple repeated cues so operators can scan the panel quickly.

| Cue | Meaning |
| --- | --- |
| `█` filled block | Active, enabled, selected, or high severity |
| `░` light block | Inactive, disabled, or background state |
| `▶` | Current cursor, selected row, start, or resume |
| `✓` | Ready, enabled, confirmed, or complete |
| `!` | Warning, alarm, or attention needed |
| `✕` | Stop, cancel, disabled, or failed |
| `⌂` | Home or homing |
| `↕` | Jog or motion |
| `◎` | Spindle |
| `◆` | Probe or tool touch-off |
| `⚙` | Settings |
| `⏸` | Feed hold |

Status colors may be used when hardware supports them:

- Green: idle, ready, connected, complete.
- Amber: paused, warning, needs confirmation, hold.
- Red: emergency stop, alarm, hard limit, failed command.
- Blue or cyan: active motion, probing, homing, connected communication.
- Gray: disabled or unavailable.

## Global Status

Every normal screen should reserve space for safety and connection state.

Large TFT status bar:

```text
┌PCB CNC MILL──────────────IDLE────────USB✓───────────────┐
│ WCS:G54  mm  ABS  Spindle:OFF  Feed:100%  E-STOP:READY │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64 status rows:

```text
┌────────────────────┐
│HOME IDLE USB✓ OK   │
│G54 mm ABS F100     │
│Spn OFF             │
└────────────────────┘
```

Required fields:

- Machine state: `IDLE`, `RUN`, `HOLD`, `HOME`, `JOG`, `PROBE`, `ALARM`,
  `E-STOP`, `OFFLINE`.
- Mainboard link state: `USB✓`, `SER✓`, `CAN✓`, `LINK?`, or `OFFLINE`.
- Active work coordinate system: `G54` through `G59`, or configured extension.
- Units: `mm` or `inch`.
- Distance mode: `ABS` or `REL`.
- Spindle state: `OFF`, `CW`, `CCW`, target rotations per minute (RPM), and
  actual RPM if available.
- Feed override, spindle override, and emergency stop state.

In alarm or emergency states, the safety state replaces the normal title.

Large TFT alarm banner:

```text
┌!!!!!!!!!!!!!!!!!!!!!!!  E-STOP ACTIVE  !!!!!!!!!!!!!!!!!┐
│ Motion and spindle disabled. Reset hardware stop first. │
└──────────────────────────────────────────────────────────┘
```

Compact alarm banner:

```text
┌────────────────────┐
│!!!! E-STOP !!!!    │
│Motion locked       │
│Reset stop first    │
└────────────────────┘
```

## Menu Tree

```text
Home
├─ Motion
│  ├─ Home Axes
│  ├─ Jog
│  ├─ Move To
│  ├─ Set Position
│  └─ Disable Steppers
├─ Coordinates
│  ├─ Position
│  ├─ Work Offsets
│  ├─ Zero Work Coordinate System
│  ├─ Set G92 Offset
│  └─ Coordinate Mode
├─ Job
│  ├─ Run File
│  ├─ Current Job
│  ├─ Feed Hold / Resume
│  ├─ Cancel Job
│  └─ Dry Run
├─ Probe
│  ├─ Probe Z
│  ├─ Tool Length
│  ├─ PCB Height Map
│  └─ Probe Settings
├─ Spindle
│  ├─ Spindle Control
│  ├─ RPM Presets
│  └─ Spindle Test
├─ Settings
│  ├─ Units and Modes
│  ├─ Machine Limits
│  ├─ Motion Settings
│  ├─ Spindle Settings
│  ├─ Display Settings
│  ├─ Communication
│  ├─ Save / Load
│  └─ Factory Reset
├─ Diagnostics
│  ├─ Inputs
│  ├─ Outputs
│  ├─ Planner / Queues
│  ├─ Mainboard Info
│  └─ Event Log
└─ Safety
   ├─ Alarm Detail
   ├─ Clear Alarm
   ├─ Unlock
   └─ Emergency Stop Test
```

## Home Screen

The home screen is the default operator view. It should show coordinates,
machine state, a small job summary, and high-value actions.

Large TFT:

```text
┌PCB CNC MILL──────────────IDLE────────USB✓───────────────┐
│ WCS:G54  mm  ABS  Spindle:OFF  Feed:100%  E-STOP:READY │
├──────────────────────────────────────────────────────────┤
│ MPos       X   124.600   Y    82.000   Z    -1.250      │
│ WPos G54   X    10.000   Y     5.000   Z     0.000      │
│                                                          │
│ Job: no file loaded                         Queue: empty │
│ Limits: X✓ Y✓ Z✓   Probe:open   Hold:clear              │
├──────────────┬──────────────┬──────────────┬────────────┤
│ ⌂ Home       │ ↕ Motion     │ ◎ Spindle    │ ◆ Probe    │
├──────────────┼──────────────┼──────────────┼────────────┤
│ Job          │ Coordinates  │ ⚙ Settings   │ Safety     │
└──────────────┴──────────────┴──────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│HOME IDLE USB✓ OK   │
│G54 mm ABS Spn OFF  │
│M X124.600 Y 82.000 │
│M Z -1.250          │
│W X 10.000 Y  5.000 │
│W Z  0.000          │
│▶Motion   Job       │
└────────────────────┘
```

Compact home alternate pages:

```text
┌────────────────────┐
│HOME 2/3 IDLE OK    │
│Lim X✓ Y✓ Z✓        │
│Prb open Hold clear │
│Queue empty         │
│▶Coords             │
│ Settings           │
│ Safety             │
└────────────────────┘
```

Home screen actions:

- `Home`: opens homing screen.
- `Motion`: opens jog and move operations.
- `Spindle`: opens manual spindle control.
- `Probe`: opens probing workflows.
- `Job`: opens loaded file and run controls.
- `Coordinates`: opens machine and work coordinate tools.
- `Settings`: opens configuration.
- `Safety`: opens alarm, emergency stop, and unlock tools.

## Alarm Home Screen

When the mainboard reports alarm or emergency stop, the home screen must collapse
to safety first.

Large TFT:

```text
┌!!!!!!!!!!!!!!!!!!!!!!!  ALARM: HARD LIMIT  !!!!!!!!!!!!!┐
│ X limit triggered while running. Motion is locked.       │
├──────────────────────────────────────────────────────────┤
│ MPos       X   126.020   Y    82.000   Z    -1.250      │
│ WPos G54   X    11.420   Y     5.000   Z     0.000      │
│                                                          │
│ Required: inspect machine, then unlock from Safety.      │
├────────────────────┬────────────────────┬───────────────┤
│ Safety Detail      │ Unlock             │ Main Menu     │
└────────────────────┴────────────────────┴───────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│ALARM HARD LIMIT    │
│X limit triggered   │
│Motion locked       │
│M X126.020          │
│W X 11.420          │
│▶Safety detail      │
│ Unlock             │
└────────────────────┘
```

Disable all motion, probing, job run, and spindle start buttons while alarm or
emergency stop is active.

## Main Menu

Large TFT:

```text
┌PCB CNC MILL──────────────IDLE────────USB✓───────────────┐
│ WCS:G54  mm  ABS  Spindle:OFF  Feed:100%  E-STOP:READY │
├──────────────────────────────────────────────────────────┤
│ ▶ ⌂ Motion and Homing                                    │
│   Coordinates and Offsets                                │
│   Job and File Control                                   │
│   ◆ Probe and Tool Setup                                 │
│   ◎ Spindle                                              │
│   ⚙ Settings                                             │
│   Diagnostics                                            │
│   Safety                                                 │
├──────────────────────────────────────────────────────────┤
│ Back                                      Select         │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│MENU IDLE USB✓ OK   │
│▶Motion/Home        │
│ Coordinates        │
│ Job/File           │
│ Probe              │
│ Spindle            │
│ Settings        1/2│
└────────────────────┘
```

```text
┌────────────────────┐
│MENU IDLE USB✓ OK   │
│ Diagnostics        │
│ Safety             │
│ Back               │
│                    │
│                    │
│                 2/2│
└────────────────────┘
```

## Motion Menu

Large TFT:

```text
┌Motion────────────────────IDLE────────USB✓───────────────┐
│ WCS:G54  mm  ABS  Spindle:OFF  Feed:100%  E-STOP:READY │
├──────────────────────────────────────────────────────────┤
│ ▶ ⌂ Home Axes                                            │
│   ↕ Jog                                                  │
│   Move To Coordinate                                     │
│   Set Position / G92                                     │
│   Disable Steppers / M18                                 │
│   Feed Hold / Resume                                     │
├──────────────────────────────────────────────────────────┤
│ Back                                      Select         │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│MOTION IDLE OK      │
│▶Home axes          │
│ Jog                │
│ Move to            │
│ Set pos/G92        │
│ Disable step       │
│ Hold/Resume        │
└────────────────────┘
```

### Home Axes

Large TFT:

```text
┌Home Axes────────────────IDLE────────USB✓───────────────┐
│ Homed: X░ Y░ Z✓   Limits: X✓ Y✓ Z✓   Probe:open         │
├──────────────────────────────────────────────────────────┤
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐ │
│ │ ⌂ Home X │ │ ⌂ Home Y │ │ ⌂ Home Z │ │ ⌂ Home All   │ │
│ └──────────┘ └──────────┘ └──────────┘ └──────────────┘ │
│                                                          │
│ Homing feed: X 800 mm/min  Y 800 mm/min  Z 150 mm/min   │
│ Pull-off: 2.000 mm                                      │
├──────────────────────────────────────────────────────────┤
│ Back                         Confirm before motion      │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│HOME AXES IDLE OK   │
│Homed X░ Y░ Z✓      │
│Lim   X✓ Y✓ Z✓      │
│▶Home X             │
│ Home Y             │
│ Home Z             │
│ Home all           │
└────────────────────┘
```

Confirm before any homing move:

```text
┌Confirm Homing────────────────────────────────────────────┐
│ Home all axes?                                           │
│                                                          │
│ The spindle must be off and the work area must be clear. │
│                                                          │
│              ┌────────────┐   ┌────────────┐            │
│              │ ✕ Cancel   │   │ ✓ Home All │            │
└──────────────┴────────────┴───┴────────────┴────────────┘
```

### Jog Screen

Jog should work well with touch or encoder. The operator chooses step size,
feed rate, and axis. Continuous jog is optional and must require a held input.

Large TFT:

```text
┌Jog───────────────────────JOG─────────USB✓───────────────┐
│ WCS:G54  mm  ABS  Spindle:OFF  Feed:100%  E-STOP:READY │
├──────────────────────────────────────────────────────────┤
│ MPos X 124.600  Y 82.000  Z -1.250                      │
│ WPos X  10.000  Y  5.000  Z  0.000                      │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Step 0.01    │ Step 0.10    │ Step 1.00 █  │ Step 10.0  │
├──────────────┴──────────────┴──────────────┴────────────┤
│              ┌────────┐                                  │
│              │  Y+    │              ┌────────┐          │
│ ┌────────┐   └────────┘   ┌────────┐ │  Z+    │          │
│ │  X-    │   ┌────────┐   │  X+    │ └────────┘          │
│ └────────┘   │  Y-    │   └────────┘ ┌────────┐          │
│              └────────┘              │  Z-    │          │
│                                      └────────┘          │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Feed -       │ Feed 500     │ Feed +       │ Back       │
└──────────────┴──────────────┴──────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│JOG IDLE OK G54     │
│Step 1.00mm F500    │
│W X 10.000          │
│W Y  5.000          │
│W Z  0.000          │
│▶X+  X-  Y+  Y-     │
│ Z+  Z-  Step Feed  │
└────────────────────┘
```

Compact jog should use a selected action row. If the display cannot fit all
axis actions in one row, split them across `Jog XY`, `Jog Z`, and `Jog Setup`
pages.

Jog fields:

- Step sizes: `0.01`, `0.10`, `1.00`, `10.0` in current units.
- Feed rate: current jog feed in `mm/min` or `inch/min`.
- Axis buttons: disabled when homing state, soft limit, alarm, probe contact,
  or mainboard state blocks the move.
- Optional continuous jog: label as `Hold Jog`, never as a normal tap button.

### Move To Coordinate

Large TFT:

```text
┌Move To───────────────────IDLE────────USB✓───────────────┐
│ Target space: WPos G54     Mode: absolute               │
├──────────────────────────────────────────────────────────┤
│ X  [   10.000 ] mm       Current WPos X    10.000       │
│ Y  [    5.000 ] mm       Current WPos Y     5.000       │
│ Z  [    0.000 ] mm       Current WPos Z     0.000       │
│ Feed [  500.0 ] mm/min                                  │
├──────────────────────────────────────────────────────────┤
│ Soft limit check: ✓ inside configured work area          │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Back         │ Edit Field   │ Preview      │ Move       │
└──────────────┴──────────────┴──────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│MOVE TO WPos G54    │
│▶X  10.000 mm       │
│ Y   5.000 mm       │
│ Z   0.000 mm       │
│ F 500.0 mm/min     │
│Soft limit OK       │
│Back Edit Move      │
└────────────────────┘
```

`Move` must confirm if Z will move downward, if spindle is on, or if target is
near a configured soft-limit boundary.

## Coordinates

Coordinates need special care because the machine has machine coordinates and
work coordinate systems.

Large TFT:

```text
┌Coordinates───────────────IDLE────────USB✓───────────────┐
│ Active WCS:G54  Units:mm  Distance:absolute             │
├──────────────────────────────────────────────────────────┤
│ Machine Position                                         │
│   X   124.600   Y    82.000   Z    -1.250               │
│ Work Position G54                                        │
│   X    10.000   Y     5.000   Z     0.000               │
│ Work Offset G54                                          │
│   X   114.600   Y    77.000   Z    -1.250               │
├──────────────────────────────────────────────────────────┤
│ ▶ Position Detail                                        │
│   Work Offsets                                           │
│   Zero X/Y/Z                                             │
│   Set G92 Temporary Offset                               │
│   Coordinate Mode                                        │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│COORD 1/3 G54 mm    │
│MPos                │
│X 124.600           │
│Y  82.000           │
│Z  -1.250           │
│▶Next page          │
│ Back               │
└────────────────────┘
```

```text
┌────────────────────┐
│COORD 2/3 G54 mm    │
│WPos                │
│X  10.000           │
│Y   5.000           │
│Z   0.000           │
│▶Zero WCS           │
│ Back               │
└────────────────────┘
```

```text
┌────────────────────┐
│COORD 3/3 G54 mm    │
│Offset              │
│X 114.600           │
│Y  77.000           │
│Z  -1.250           │
│▶Work offsets       │
│ Back               │
└────────────────────┘
```

### Position Detail

```text
┌Position Detail───────────────────────────────────────────┐
│ Space       X           Y           Z                    │
├──────────────────────────────────────────────────────────┤
│ MPos    124.600      82.000      -1.250                 │
│ G54      10.000       5.000       0.000                 │
│ G55       0.000       0.000       0.000                 │
│ G56       0.000       0.000       0.000                 │
│ G57       0.000       0.000       0.000                 │
│ G58       0.000       0.000       0.000                 │
│ G59       0.000       0.000       0.000                 │
├──────────────────────────────────────────────────────────┤
│ Back                 Select WCS                 Refresh │
└──────────────────────────────────────────────────────────┘
```

### Work Offsets

Large TFT:

```text
┌Work Offsets──────────────IDLE────────USB✓───────────────┐
│ Active: G54   Units:mm                                  │
├──────────────────────────────────────────────────────────┤
│ ▶ G54  X 114.600  Y 77.000  Z -1.250   active █         │
│   G55  X   0.000  Y  0.000  Z  0.000                    │
│   G56  X   0.000  Y  0.000  Z  0.000                    │
│   G57  X   0.000  Y  0.000  Z  0.000                    │
│   G58  X   0.000  Y  0.000  Z  0.000                    │
│   G59  X   0.000  Y  0.000  Z  0.000                    │
├──────────────────────────────────────────────────────────┤
│ Back              Activate              Edit            │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│OFFSETS G54 mm      │
│▶G54 active         │
│ G55                │
│ G56                │
│ G57                │
│ G58             1/2│
│Back Act Edit       │
└────────────────────┘
```

### Zero Work Coordinate System

Large TFT:

```text
┌Zero G54──────────────────IDLE────────USB✓───────────────┐
│ Set current tool position as zero for active WCS.        │
├──────────────────────────────────────────────────────────┤
│ Current WPos G54   X 10.000   Y 5.000   Z 0.000         │
│ Current MPos       X124.600   Y82.000   Z-1.250         │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Zero X       │ Zero Y       │ Zero Z       │ Zero All   │
├──────────────┴──────────────┴──────────────┴────────────┤
│ This changes the active work offset, not machine home.   │
├──────────────────────────────────────────────────────────┤
│ Back                                      Confirm        │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│ZERO G54 IDLE OK    │
│Set current pos     │
│as WCS zero         │
│W X10 Y5 Z0         │
│▶Zero X             │
│ Zero Y  Zero Z     │
│ Zero all           │
└────────────────────┘
```

## Job Control

The job screen may use streamed host G-code, SD card files, or future display
storage. Keep the controls independent of the transport.

Large TFT:

```text
┌Job───────────────────────RUN─────────USB✓───────────────┐
│ File: top_copper.nc                                      │
├──────────────────────────────────────────────────────────┤
│ Progress ███████████░░░░░░░░░  54%                      │
│ Line 1842 / 3401     Elapsed 00:12:34     Remain 00:10  │
│ WPos X 10.000  Y 5.000  Z -0.080   Feed 240 mm/min      │
│ Spindle CW 12000 RPM                                     │
├──────────────┬──────────────┬──────────────┬────────────┤
│ ⏸ Hold       │ ▶ Resume     │ ✕ Cancel     │ Overrides  │
├──────────────┼──────────────┼──────────────┼────────────┤
│ File List    │ Dry Run      │ Status       │ Back       │
└──────────────┴──────────────┴──────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│JOB RUN USB✓ OK     │
│top_copper.nc       │
│54% L1842/3401      │
│00:12 R00:10        │
│W X10 Y5 Z-0.080    │
│▶Hold Resume Cancel │
│Overrides Back      │
└────────────────────┘
```

### File List

Large TFT:

```text
┌Run File──────────────────IDLE────────USB✓───────────────┐
│ Source: SD card                                          │
├──────────────────────────────────────────────────────────┤
│ ▶ top_copper.nc                      128 KB              │
│   drill.nc                            34 KB              │
│   outline.nc                          22 KB              │
│   height_map_probe.nc                  8 KB              │
├──────────────────────────────────────────────────────────┤
│ Back              Refresh              Open          Run │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│FILES SD IDLE OK    │
│▶top_copper.nc      │
│ drill.nc           │
│ outline.nc         │
│ height_map.nc      │
│                 1/1│
│Back Refresh Run    │
└────────────────────┘
```

`Run` must confirm file name, units, estimated bounds if known, and spindle
start behavior.

### Overrides

```text
┌Overrides─────────────────RUN─────────USB✓───────────────┐
│ Feed override      ██████████░░░░░░░░ 100%              │
│ Spindle override   ██████████░░░░░░░░ 100%              │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Feed -10%    │ Feed +10%    │ Spin -10%    │ Spin +10%  │
├──────────────┴──────────────┴──────────────┴────────────┤
│ Reset overrides                                          │
└──────────────────────────────────────────────────────────┘
```

## Probe

Probe screens are for PCB zeroing, tool touch-off, and future height mapping.
Every probe move must show probe input state and target travel.

Large TFT:

```text
┌Probe─────────────────────IDLE────────USB✓───────────────┐
│ Probe input: open     Last probe: none                  │
├──────────────────────────────────────────────────────────┤
│ ▶ Probe Z                                                 │
│   Tool Length / Touch Plate                              │
│   PCB Height Map                                         │
│   Probe Settings                                         │
├──────────────────────────────────────────────────────────┤
│ Back                                      Select         │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│PROBE IDLE OK       │
│Prb open            │
│Last none           │
│▶Probe Z            │
│ Tool length        │
│ PCB height map     │
│ Settings           │
└────────────────────┘
```

### Probe Z

Large TFT:

```text
┌Probe Z───────────────────PROBE───────USB✓───────────────┐
│ Probe: open   Plate thickness: 1.600 mm                 │
├──────────────────────────────────────────────────────────┤
│ Start Z        5.000 mm                                  │
│ Max travel    10.000 mm                                  │
│ Feed fast    100.0 mm/min                                │
│ Feed slow     25.0 mm/min                                │
│ Result        not run                                    │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Back         │ Edit         │ Test Probe   │ Start      │
└──────────────┴──────────────┴──────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│PROBE Z IDLE OK     │
│Prb open Plate 1.60 │
│Start Z 5.000       │
│Travel 10.000       │
│Fast 100 Slow 25    │
│▶Start              │
│Back Edit Test      │
└────────────────────┘
```

### PCB Height Map

```text
┌PCB Height Map────────────IDLE────────USB✓───────────────┐
│ Area G54: X 0..80 mm  Y 0..60 mm   Grid 9 x 7           │
├──────────────────────────────────────────────────────────┤
│ Points complete: 00 / 63                                │
│ Last point: none                                         │
│ Z safe: 2.000 mm     Probe feed: 50.0 mm/min            │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Back         │ Edit Area    │ Preview      │ Start Map  │
└──────────────┴──────────────┴──────────────┴────────────┘
```

## Spindle

Manual spindle control must be explicit and disabled when the mainboard reports
that spindle start is unsafe.

Large TFT:

```text
┌Spindle───────────────────IDLE────────USB✓───────────────┐
│ State: OFF       Target: 12000 RPM       Actual: n/a     │
├──────────────────────────────────────────────────────────┤
│ RPM preset                                                  │
│ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐ │
│ │  8000    │ │ 12000 █  │ │ 16000    │ │ Custom       │ │
│ └──────────┘ └──────────┘ └──────────┘ └──────────────┘ │
├──────────────┬──────────────┬──────────────┬────────────┤
│ CW / M3      │ CCW / M4     │ Stop / M5    │ Test Pulse │
├──────────────┴──────────────┴──────────────┴────────────┤
│ Warning: verify tool and work holding before start.      │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│SPINDLE IDLE OK     │
│State OFF           │
│Target 12000 RPM    │
│Actual n/a          │
│▶CW/M3              │
│ CCW/M4  Stop/M5    │
│RPM Edit Back       │
└────────────────────┘
```

`CW`, `CCW`, and `Test Pulse` require confirmation if the spindle is off.

## Settings

Settings screens should show current values, configured bounds, and whether a
restart or save is required.

Large TFT:

```text
┌Settings──────────────────IDLE────────USB✓───────────────┐
│ Profile: default_pcb_mill        Unsaved changes: no    │
├──────────────────────────────────────────────────────────┤
│ ▶ Units and Modes                                        │
│   Machine Limits                                         │
│   Motion Settings                                        │
│   Spindle Settings                                       │
│   Probe Settings                                         │
│   Display Settings                                       │
│   Communication                                          │
│   Save / Load                                            │
│   Factory Reset                                          │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│SETTINGS IDLE OK    │
│▶Units/Modes        │
│ Machine limits     │
│ Motion             │
│ Spindle            │
│ Probe           1/2│
│Back                │
└────────────────────┘
```

```text
┌────────────────────┐
│SETTINGS IDLE OK    │
│ Display            │
│ Communication      │
│ Save/Load          │
│ Factory reset      │
│                 2/2│
│Back                │
└────────────────────┘
```

### Units and Modes

Large TFT:

```text
┌Units and Modes───────────────────────────────────────────┐
│ Units                 [mm]                               │
│ Distance mode         [absolute]                         │
│ Arc mode              [IJK incremental]                  │
│ Default WCS           [G54]                              │
│ Position decimals     [3]                                │
├──────────────────────────────────────────────────────────┤
│ Back                 Edit                 Save           │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│UNITS/MODES         │
│▶Units mm           │
│ Dist absolute      │
│ Arc IJK incr       │
│ Default WCS G54    │
│ Decimals 3         │
│Back Edit Save      │
└────────────────────┘
```

### Machine Limits

Large TFT:

```text
┌Machine Limits────────────────────────────────────────────┐
│ Units:mm                                                 │
├──────────────────────────────────────────────────────────┤
│ X min [  0.000]   X max [120.000]   Home dir [-]         │
│ Y min [  0.000]   Y max [100.000]   Home dir [-]         │
│ Z min [-20.000]   Z max [  0.000]   Home dir [+]         │
│ Soft limits [enabled]   Hard limits [enabled]            │
├──────────────────────────────────────────────────────────┤
│ Back                 Edit                 Save           │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│LIMITS mm 1/2       │
│▶X min   0.000      │
│ X max 120.000      │
│ Y min   0.000      │
│ Y max 100.000      │
│Soft limits enabled │
│Back Edit Save      │
└────────────────────┘
```

### Motion Settings

```text
┌Motion Settings───────────────────────────────────────────┐
│ Steps per mm      X [ 80.000] Y [ 80.000] Z [400.000]   │
│ Max feed mm/min   X [3000.0]  Y [3000.0]  Z [ 300.0]    │
│ Accel mm/s^2      X [ 200.0]  Y [ 200.0]  Z [  50.0]    │
│ Junction speed    [  5.000] mm/min                      │
│ Homing feed       X [800.0]   Y [800.0]   Z [150.0]     │
├──────────────────────────────────────────────────────────┤
│ Back                 Edit                 Save           │
└──────────────────────────────────────────────────────────┘
```

### Spindle Settings

```text
┌Spindle Settings──────────────────────────────────────────┐
│ Min RPM [  5000]    Max RPM [ 24000]                    │
│ Default RPM [12000]                                     │
│ PWM min [  0.0%]    PWM max [100.0%]                    │
│ Spin-up delay [ 2.0 s]                                  │
│ Direction control [CW only]                             │
├──────────────────────────────────────────────────────────┤
│ Back                 Edit                 Save           │
└──────────────────────────────────────────────────────────┘
```

### Display Settings

```text
┌Display Settings──────────────────────────────────────────┐
│ Brightness         ███████████░░░░░  70%                │
│ Backlight timeout  [30 s]                               │
│ Buzzer             [enabled]                            │
│ Touch calibration  [run]                                │
│ Encoder direction  [normal]                             │
│ Theme              [dark]                               │
├──────────────────────────────────────────────────────────┤
│ Back                 Edit                 Save           │
└──────────────────────────────────────────────────────────┘
```

### Save / Load

```text
┌Save / Load───────────────────────────────────────────────┐
│ Current profile: default_pcb_mill                        │
│ Unsaved changes: yes                                     │
├──────────────────────────────────────────────────────────┤
│ ▶ Save to mainboard                                      │
│   Reload from mainboard                                  │
│   Export to display storage                              │
│   Import from display storage                            │
├──────────────────────────────────────────────────────────┤
│ Back                                      Select         │
└──────────────────────────────────────────────────────────┘
```

## Diagnostics

Diagnostics are read-only unless a row clearly says it drives an output.

Large TFT:

```text
┌Diagnostics───────────────IDLE────────USB✓───────────────┐
│ ▶ Inputs                                                 │
│   Outputs                                                │
│   Planner / Queues                                       │
│   Mainboard Info                                         │
│   Event Log                                              │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│DIAG IDLE OK        │
│▶Inputs             │
│ Outputs            │
│ Planner/Queues     │
│ Mainboard info     │
│ Event log          │
│Back                │
└────────────────────┘
```

### Inputs

```text
┌Inputs────────────────────IDLE────────USB✓───────────────┐
│ Emergency stop       READY                              │
│ X limit              open                               │
│ Y limit              open                               │
│ Z limit              open                               │
│ Probe                open                               │
│ Feed hold            clear                              │
│ Door / cover         n/a                                │
├──────────────────────────────────────────────────────────┤
│ Back                                      Refresh        │
└──────────────────────────────────────────────────────────┘
```

### Planner / Queues

```text
┌Planner / Queues──────────RUN─────────USB✓───────────────┐
│ Planner blocks       12 / 32                            │
│ Step segments        18 / 48                            │
│ Command queue         2 / 16                            │
│ Display queue         1 / 16                            │
│ RX bytes             64 / 256                           │
│ Last error           none                               │
└──────────────────────────────────────────────────────────┘
```

### Mainboard Info

```text
┌Mainboard Info────────────────────────────────────────────┐
│ Firmware       pcb-cnc-mill 0.1                          │
│ Mainboard      btt_skr_mini_e3_v3                        │
│ Display        btt_tft35_e3                              │
│ Toolhead       none                                      │
│ Uptime         00:42:17                                  │
│ Link           USB serial                                │
└──────────────────────────────────────────────────────────┘
```

## Safety

Safety screens must be reachable from every screen and must always show the
current safety state.

Large TFT:

```text
┌Safety────────────────────IDLE────────USB✓───────────────┐
│ Emergency stop: READY     Alarm: clear                  │
├──────────────────────────────────────────────────────────┤
│ ▶ Alarm Detail                                           │
│   Clear Alarm                                            │
│   Unlock Motion                                          │
│   Feed Hold                                              │
│   Emergency Stop Test                                    │
├──────────────────────────────────────────────────────────┤
│ Back                                      Select         │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│SAFETY IDLE OK      │
│E-stop READY        │
│Alarm clear         │
│▶Alarm detail       │
│ Clear alarm        │
│ Unlock motion      │
│Hold E-stop test    │
└────────────────────┘
```

### Alarm Detail

Large TFT:

```text
┌Alarm Detail──────────────────────────────────────────────┐
│ State       HARD LIMIT                                   │
│ Source      X limit                                      │
│ Time        00:41:02                                     │
│ Action      Motion stopped, spindle stopped              │
│ Recovery    Inspect machine, move clear, then unlock     │
├──────────────────────────────────────────────────────────┤
│ Back                 Clear Alarm          Unlock         │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│ALARM DETAIL        │
│State HARD LIMIT    │
│Source X limit      │
│Action stopped      │
│Inspect machine     │
│▶Unlock             │
│Back Clear          │
└────────────────────┘
```

### Emergency Stop Test

```text
┌Emergency Stop Test───────────────────────────────────────┐
│ This test checks that the display sees the stop state.   │
│ It must not start motion or spindle.                     │
│                                                          │
│ Press the hardware emergency stop now.                   │
│                                                          │
│ Input state: READY                                       │
├──────────────────────────────────────────────────────────┤
│ Back                                      Start Test     │
└──────────────────────────────────────────────────────────┘
```

## Edit Screen Pattern

Use one edit pattern for all numeric and option fields.

Large TFT:

```text
┌Edit Value────────────────────────────────────────────────┐
│ Field: X max travel                                      │
│ Current: 120.000 mm                                      │
│ New:     120.000 mm                                      │
│ Range:     1.000 .. 500.000 mm                           │
├──────────────────────────────────────────────────────────┤
│ Step: 1.000 mm                                           │
│                                                          │
│          ┌────────┐   ┌────────┐   ┌────────┐           │
│          │   -    │   │  Step  │   │   +    │           │
│          └────────┘   └────────┘   └────────┘           │
├──────────────────────────────────────────────────────────┤
│ Cancel                                      Apply        │
└──────────────────────────────────────────────────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│EDIT X max travel   │
│Cur 120.000 mm      │
│New 120.000 mm      │
│Range 1..500 mm     │
│Step 1.000          │
│▶-  +  Step         │
│Cancel Apply        │
└────────────────────┘
```

Encoder behavior:

- Rotate: change selected digit, option, or row.
- Press: activate row or enter field edit.
- Long press: back or cancel when no safer action is configured.

Touch behavior:

- Tap: activate button or field.
- Hold: allowed only for continuous jog and must stop when touch is released.

## Confirmation Pattern

Large TFT:

```text
┌Confirm Action────────────────────────────────────────────┐
│ Start spindle clockwise at 12000 RPM?                    │
│                                                          │
│ Tool and work holding must be secure.                    │
│                                                          │
│              ┌────────────┐   ┌────────────┐            │
│              │ ✕ Cancel   │   │ ✓ Start    │            │
└──────────────┴────────────┴───┴────────────┴────────────┘
```

Compact 128x64:

```text
┌────────────────────┐
│CONFIRM SPINDLE     │
│Start CW 12000 RPM? │
│Check tool and work │
│holding first.      │
│▶Cancel             │
│ Start              │
│                    │
└────────────────────┘
```

Use confirmations for:

- Homing one or more axes.
- Jog continuous mode.
- Move to coordinate.
- Zeroing a work coordinate system.
- Editing G92 temporary offsets.
- Starting spindle.
- Running a file.
- Starting probe or height map.
- Clearing alarm or unlocking motion.
- Saving settings.
- Factory reset.

## Mainboard Data Needed

The display menu needs these status fields from the mainboard:

- Machine state and safety state.
- Emergency stop, alarm, hard limit, feed hold, and probe input state.
- Machine position and active work coordinate position.
- Active work coordinate system and all stored work offsets.
- Units, distance mode, feed rate, spindle state, spindle target RPM, and
  optional actual RPM.
- Homed state per axis.
- Machine limits, soft-limit state, max feed, acceleration, steps per unit, and
  homing settings.
- Planner, step segment, command, communication, and display queue depth.
- Job source, file name, line number, progress, elapsed time, and current
  command if available.
- Mainboard identity, firmware version, board name, uptime, and link type.

The display must treat this data as a snapshot. If the mainboard link is stale,
show `OFFLINE`, disable motion and spindle actions, and keep local safety
controls visible.

## Implementation Notes

- Shared display code should own the menu state machine, dirty regions, field
  editing, and command generation.
- Shared display code should expose layout-independent screens and actions.
  Board or display-profile code should choose whether a screen is rendered as a
  large touch page, compact 128x64 page, or another future profile.
- Standalone display board HAL code should own its display-board clock setup,
  LCD controller setup, physical drawing primitives, touch or encoder sampling,
  backlight, buzzer, local LEDs, and communication link to the mainboard.
- Mainboard-attached display module code should own only panel-local hardware
  such as compact LCD pins, encoder inputs, click buttons, backlight, sounder,
  and LEDs. It must not own machine state, motion validation, mainboard clocks,
  reset handlers, linker scripts, or separate firmware update behavior.
- Display refresh work should be split into bounded regions so input, buzzer,
  communication, and safety state polling stay responsive.
- Compact display redraw should be line-based or tile-based so encoder response
  stays fast. It should not redraw the full 128x64 frame for every encoder
  tick unless the target hardware can do that without input lag.
- Repeated status updates may coalesce; safety state updates must not be hidden
  behind low-priority redraw work.
- Mainboard commands from the display should be typed operations where possible,
  even when they map to G-code such as `G28`, `G90`, `G91`, `G92`, `M3`, `M4`,
  `M5`, `M18`, `M112`, or status reports.
- When the display sends raw G-code for compatibility, it must still present
  the operation as a clear CNC action to the user.
