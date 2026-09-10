# PI flow control

MANUAL applies the committed manual fan value while the vent relay is ON. AUTO
accepts only the committed `flowSetpointLpm`; editing `tFlowSet` with `+` or
`-` cannot affect PI until `bFlowApply` is pressed. Its feedback is always
`filteredFlow1Lpm + filteredFlow2Lpm`. Both meters must be valid. A missing,
stale, or invalid meter is not treated as zero: AUTO exits to MANUAL and keeps
the current safe fan command.

In AUTO, `bVentApply` commits the pending manual fan value for a later return
to MANUAL but cannot override the current PI PWM output. The display and
Serial status distinguish the pending/applied manual setting from the actual
PI command and raw PWM value.

Initial settings: `Kp/Cp = 0.5`, `Ti = 100 s`, update period 1 s, output
0…100%, and output slew limit 5 percentage-points/s. The normalised-percent
error is:

`100 * (appliedFlowSetpointLpm - (filteredFlow1Lpm + filteredFlow2Lpm)) / max(appliedFlowSetpointLpm, PI_MIN_NORMALIZATION_LPM)`

The integrator has anti-windup and MANUAL→AUTO uses a bumpless transfer from
the current actual fan command. Before tuning, confirm that increasing fan
command increases the total measured flow; do not tune PI with a failed meter.
