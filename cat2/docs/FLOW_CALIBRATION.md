# Flow conversion and air-density correction

The conversion is configured in firmware; no guessed pulses-per-litre table is
used. Frequency `f` is in Hz and output `Q` is in L/min.

```text
Q1 = (rho24 / rhoOut) * max(0, -3.334481 + 1.885698 * f1)
Q2 = (rho24 / rhoOut) * max(0,  5.348758 + 1.453538 * f2)
```

The positive intercept of meter 2 is intentionally overridden: after the
3-second zero-flow timeout, its output is exactly `0.0 L/min` even though the
mathematical line would otherwise be positive.

`rho24` is humid-air density at 24 °C, 101325 Pa and RH=0.5. `rhoOut` is
humid-air density at the outlet-air temperature, 101325 Pa and RH=1.0. Both are
obtained from CoolProp's [Humid Air Properties API](https://coolprop.org/coolprop/HighLevelAPI.html#humid-air-properties)
using `1 / HAPropsSI("Vha", ...)`.

The Mega does not run CoolProp. Host tooling creates the checked-in AVR lookup
table [air_density_table.h](../include/generated/air_density_table.h): -20…80
°C inclusive, 1 °C step, stored in Flash with `PROGMEM`. Firmware interpolates
between table entries. Temperatures outside this range make conversion invalid
and block AUTO; the most recent valid outlet temperature may bridge a single
read failure for at most 30 seconds.

## Reproducing the table

```powershell
python -m pip install -r requirements-tools.txt
python tools/generate_air_density_table.py
```

The pinned host-only dependency is `CoolProp==7.2.0`. It is deliberately not a
PlatformIO / Arduino library. Regenerate and review the generated header after
changing the equation, pressure, humidity, temperature range, or CoolProp
version.
