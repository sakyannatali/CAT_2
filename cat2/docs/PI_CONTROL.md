# AUTO flow model (PI retired)

This file retains its historical name so existing links continue to work. The
firmware contains **no PI controller**, no integral state, no rate limiter and
no flow-meter feedback path.

AUTO is feed-forward. Once a target has been committed, the firmware evaluates
the experimental total-flow model for the currently applied signed gate and
uses a bounded binary search over fan power 0…100% to find the closest target.
It sends that result through the existing central fan/actuator path only; it
does not switch the vent relay. A gate edit has no effect until Apply, then
AUTO immediately recalculates the model command.

The target choices are `NONE`, `30`, `35`, …, `250 L/min`. `NONE` commands
model power `0%` and preserves the relay state. AUTO can be selected even if
the vent relay is OFF; in that case the saved derived command is visible in
diagnostics, but the applied fan power and estimated actual flow are zero.

The model is experimental from 5% through 60% fan command. A solved result
below 5% is reported as `BELOW CAL RANGE`; above 60% it is `EXTRAPOLATED`.
If the requested total is greater than the model predicts at 100%, the fan is
commanded to 100% and the status is `UNREACHABLE`. These are diagnostic
honesty states, not sensor-fault or relay-control commands.

`fPiOutput` is an unchanged HMI component name only. Its displayed value is
the actual applied AUTO model power, not a PI output. On AUTO→MANUAL the
current actual power becomes the applied/pending manual power, avoiding a PWM
step.
