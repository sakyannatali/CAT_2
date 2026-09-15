# Diagnostics

Use the 9600-baud monitor. Commands are bounded to 95 characters.

| Command | Result |
|---|---|
| `status` | Actuators, all applied/pending values, DS18B20/GY-906 health and I²C timeout/recovery counters |
| `flow`, `flow raw`, `flow density`, `flow reset` | Flow frequency, L/min conversion, density and counters |
| `control manual\|auto` | Select actual mode |
| `control setpoint <L/min>` | Immediately set an applied setpoint from Serial, range 0…100 |
| `control status` | Both filtered flows, total, setpoint, error, P/I, raw/clamped/rate-limited PI outputs, actual command, density-temperature validity/age and AUTO block reason |
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

Each temperature status line contains the last-good value, `valid`, `stale`,
`fail_count`, age and last error. One or two failures retain the last-good
value; `TEMP_FAIL_COUNT_LIMIT=3` failures or an age of
`TEMP_STALE_TIMEOUT_MS=2500` makes the sensor stale. Recovery is attempted at
most once per second. A stale outlet temperature produces `TEMP SENSOR STALE`,
invalidates density correction and causes AUTO→MANUAL without changing the
current fan command. The I²C summary reports timeout/recovery counts and the
last PCA9547-ACK result.
