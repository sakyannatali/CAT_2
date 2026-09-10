# PI flow control

MANUAL applies the committed manual fan value. AUTO accepts only the committed `flowSetpointLpm`; editing `tFlowSet` with `+` or `-` cannot affect PI until `bFlowApply` is pressed. The default feedback source is `AVERAGE_OF_VALID`; one valid meter is sufficient and a missing meter is never a zero.

In AUTO, `bVentApply` commits the pending manual fan value for a later return to MANUAL but cannot override the current PI PWM output. Flow feedback loss keeps the safe controller behavior and exits AUTO without commanding 100%.

Initial settings: `Kp/Cp = 0.5`, `Ti = 100 s`, update period 1 s, output 0…100%, and output slew limit 5 percentage-points/s. The normalised error is:

`(appliedFlowSetpointLpm - filteredFlowLpm) / max(appliedFlowSetpointLpm, PI_MIN_NORMALIZATION_LPM)`

The integrator has anti-windup and MANUAL→AUTO uses a bumpless transfer.
