# PI flow control

MANUAL applies the committed manual fan value while the vent relay is ON. AUTO
accepts only the committed `flowSetpointLpm`; editing `tFlowSet` with `+` or
`-` cannot affect PI until `bFlowApply` is pressed. Its feedback is always
`filteredFlow1Lpm + filteredFlow2Lpm`. Both meters must be valid. A missing,
stale, or invalid meter is not treated as zero: AUTO exits to MANUAL and keeps
the current safe fan command.

In AUTO, `bVentApply` cannot override the current PI PWM output. `tVentSet`
displays `AUTO`, rather than a manual percentage. On AUTO→MANUAL, the actual
fan output becomes both the applied and pending manual setting, so the next
MANUAL output is bumpless. `fPiOutput` keeps its current HMI name and means
**AUTO power**: the actual PI fan command while AUTO is active.

Initial settings: `Kp/Cp = 0.5`, `Ti = 100 s`, update period 1 s, output
0…100%. Its asymmetric output limits are +10 percentage-points/s rising and
−20 percentage-points/s falling. The normalised-percent error is:

`100 * (appliedFlowSetpointLpm - (filteredFlow1Lpm + filteredFlow2Lpm)) / max(appliedFlowSetpointLpm, PI_MIN_NORMALIZATION_LPM)`

`control status` exposes the raw PI result, its 0…100% clamped result, and the
rate-limited output separately. The integrator has anti-windup and
MANUAL→AUTO uses a bumpless transfer from the current actual fan command.
Kp remains 0.5 and Ti remains 100 s. Before tuning, confirm that increasing
fan command increases the total measured flow; do not tune PI with a failed
meter.
