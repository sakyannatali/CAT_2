# PI flow control

Manual mode accepts fan power directly. AUTO accepts a target in **L/min** and
uses the latest non-blocking, 5-second filtered flow from sensor 1, sensor 2,
or the average of valid sensors. The default is `AVERAGE_OF_VALID`; one valid
sensor is sufficient and a missing one is never treated as zero.

AUTO is allowed only when the conversion is configured, the selected feedback
is valid, and the vent relay is on. It starts in MANUAL after reboot. Loss of
feedback freezes the last output and returns to MANUAL with a diagnostic error;
it never drives the fan to 100% because a sensor disappeared.

Initial settings are `Kp/Cp = 0.5`, `Ti = 100 s`, one-second PI updates,
0–100% clamp, and a 5 percentage-point/s output rate limit. Error is relative:

`(setpointLpm - filteredFlowLpm) / max(setpointLpm, PI_MIN_NORMALIZATION_LPM)`

The integrator has anti-windup and transfer from MANUAL to AUTO is bumpless:
the present manual fan output is used as the initial AUTO output.

1. Verify the actual flow direction in MANUAL.
2. Start with Kp=0.5 and Ti=100 s.
3. Increase Kp in small steps only after observing several 5-second windows.
4. If it oscillates, reduce Kp or increase Ti.
5. Do not tune with an invalid outlet-temperature correction or faulty flow sensor.
