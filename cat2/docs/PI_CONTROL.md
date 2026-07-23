# PI flow control

Manual mode accepts fan power directly. AUTO takes a kg/h setpoint and uses a configurable sensor 1, sensor 2, or mean of valid sensors. It is permitted only with valid calibration, a fresh selected flow value, and the vent relay enabled.

Initial values: `Kp = 0.5`, `Ti = 100 s`, update period 1 s, output 0–100%, and rate limit 5 percentage points/s. The error is normalised by `PI_FLOW_FULL_SCALE_KG_H`; the integrator has anti-windup and is frozen when a saturated output would be driven further outward. MANUAL→AUTO starts from the manual output.

1. Calibrate both meters.
2. In manual mode verify that increasing requested power changes flow in the expected direction.
3. Start at Ti=100 s and Kp=0.5.
4. Increase Kp gradually.
5. If it oscillates, reduce Kp or increase Ti.
6. Never tune PI with uncalibrated meters.

Sensor loss freezes the controller and changes to MANUAL; it never commands 100% due to missing data.
