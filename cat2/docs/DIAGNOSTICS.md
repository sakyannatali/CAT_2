# Diagnostics

Use the 9600-baud monitor. Commands are bounded to 95 characters.

| Command | Result |
|---|---|
| `status` | Actuators, sensor states, timer, and all applied/pending edit values |
| `flow`, `flow raw`, `flow density`, `flow reset` | Flow frequency, L/min conversion, density and counters |
| `control manual\|auto` | Select actual mode |
| `control setpoint <L/min>` | Immediately set an applied setpoint from Serial, range 0…100 |
| `control status` | Both filtered flows, their required total, setpoint, L/min and normalised error, P/I/raw/rate-limited PI outputs, manual fan applied/pending, actual command, raw PWM, relay, inversion and AUTO block reason |
| `control kp <value>`, `control ti <s>` | PI tuning for this boot |
| `timer start\|pause\|reset\|status` | Arduino timer |
| `i2c scan`, `i2c mux`, `tof …` | Bus and ToF diagnosis |

`status` reports three independent edit records: applied, pending, editing, age and remaining time. The five-second timeout is calculated from each parameter's most recent `+`/`-` event. Expiry discards pending data; Apply when not editing is deliberately a no-op.

`Fan command: <percent>% -> PWM <0..255>` is emitted only when the raw PWM
value changes. It is the direct trace of the only hardware PWM path; with an
OFF vent relay the actual command remains `0%` even when a manual value is
stored.

AUTO never selects one meter or an average. It requires valid filtered values
from both meters and uses their sum; per-meter feedback selection is not part
of the command interface.
