# Diagnostics

Use the 9600-baud monitor. Commands are bounded to 95 characters.

| Command | Result |
|---|---|
| `status` | Actuators, sensor states, timer, and all applied/pending edit values |
| `flow`, `flow raw`, `flow density`, `flow reset` | Flow frequency, L/min conversion, density and counters |
| `control manual\|auto` | Select actual mode |
| `control setpoint <L/min>` | Immediately set an applied setpoint from Serial, range 0…100 |
| `control source 1\|2\|avg`, `control status` | Feedback, PI terms, applied setpoint and flow pending state |
| `control kp <value>`, `control ti <s>` | PI tuning for this boot |
| `timer start\|pause\|reset\|status` | Arduino timer |
| `i2c scan`, `i2c mux`, `tof …` | Bus and ToF diagnosis |

`status` reports three independent edit records: applied, pending, editing, age and remaining time. The five-second timeout is calculated from each parameter's most recent `+`/`-` event. Expiry discards pending data; Apply when not editing is deliberately a no-op.
