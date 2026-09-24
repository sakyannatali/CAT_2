# Diagnostics

Use the 9600-baud serial monitor. Input is bounded to 95 characters; an unknown
or invalid command prints the compact help text.

| Command | Result |
|---|---|
| `status` | Central actuator state, pending edits, timer, temperature/ToF health, and model status |
| `flow model` | Applied model estimate: Q1, Q2, total, target, derived fan command, actual fan command, relay and raw PWM |
| `flow model <power> <gate>` | Diagnostic-only model calculation; power is 0…100 and gate is -100…100; no actuator changes |
| `control manual`, `control auto` | Select MANUAL or model AUTO |
| `control setpoint NONE` (or `0`) | Commit no flow demand, hence AUTO model PWM 0% without operating the relay |
| `control setpoint 30` … `250` | Commit a valid five-L/min target step |
| `control status` | Same detailed model-control status as `flow model` |
| `timer start\|pause\|reset\|status` | Arduino timer |
| `i2c scan`, `i2c mux`, `tof …` | Bus and ToF diagnosis |
| `debug off\|sensors\|all` | Disable reports, sensor/ToF report, or sensor/ToF plus model report every second |

`flow`, `flow raw`, `flow density`, `flow reset`, PI tuning commands, pulse
counters and Hz output have been intentionally removed because the hardware
meters are disconnected. The `flow model` diagnostic is an estimate, not a
replacement for a sensor.

Each edit record reports applied value, pending value, editing state, age and
remaining time. The five-second timeout is measured from the latest `+`/`-`
event. An expired edit is discarded; Apply while not editing is a no-op.

`Fan command: <percent>% -> PWM <0..255>` appears only when the actual raw PWM
value changes. It is the trace of the sole hardware PWM path. With vent relay
OFF, applied fan power and calculated flow are zero even if a manual or model
command has been saved.

Temperature and ToF fault information remains independent from model AUTO.
A stale temperature is displayed and logged, but does not alter model PWM.
