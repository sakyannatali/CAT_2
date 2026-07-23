# Flow-meter calibration

Do not use guessed pulses-per-litre or mass conversion factors. Collect paired measurements `{ frequencyHz, massFlowKgH }` for each meter, sorted by strictly increasing frequency.

1. Edit `FLOW1_CALIBRATION` and `FLOW2_CALIBRATION` in `cat2/include/config.h`.
2. Replace the duplicate zero placeholders with at least two real points per table.
3. Set `FLOW_CALIBRATED` to `true` only after review.
4. Build and use `flow raw` to confirm pulse frequency, period, age and stale status.

At boot the firmware validates each table (two or more points, increasing frequency, no duplicates). Invalid tables keep AUTO disabled. Interpolation is linear, with endpoint clamping. The UI reports Hz until calibration is valid, then kg/h.
