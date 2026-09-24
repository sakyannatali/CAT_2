# Experimental open-loop flow model

This firmware does not measure flow. It estimates two output flows from the
actual applied fan power `P` (0…100%) and the applied signed gate command.
When the vent relay is off, firmware passes `P=0`, so all three estimates are
exactly zero regardless of a stored manual value.

For duct diameter 12.5 mm, the fixed conversion is:

```text
Q_L/min = 7.363107781851078 × v_m/s
v(P) = A × (1 - exp(-k × P))
```

The exact direct experimental curves are in
[`../include/config.h`](../include/config.h): output 1 and 2 at gate `-100`,
`-50`, and `0`. No pulse coefficient, air-density correction or temperature
correction is used.

For `-100…0`, each output is linearly interpolated between its individual
direct curves. For example, `-30` is `0.6 × Q(-50) + 0.4 × Q(0)`. Positive
positions use the agreed derived geometry:

- `+50`: the two outputs at `-30` swapped;
- `+100`: the two outputs at `-100` swapped;
- `0…+50` and `+50…+100`: linear interpolation between those endpoints.

This intentionally does **not** make `+50` equivalent to `-50`.

## Validity labels

The formula is an experimental reference, not a guarantee of stable airflow.
The qualified range is 5…60% fan power. Model AUTO labels its inverse result:

| Label | Meaning |
|---|---|
| `NONE` | No committed flow demand; model command is 0% |
| `OK` | Derived power lies in 5…60% |
| `BELOW CAL RANGE` | Derived power is below 5%; it is computed but not presented as validated operation |
| `EXTRAPOLATED` | Derived power is above 60% |
| `UNREACHABLE` | Demand exceeds the model estimate at 100%; AUTO selects 100% |

`flow model` prints the current applied estimate. `flow model 30 -30` evaluates
an arbitrary power/gate point without changing any actuator.
