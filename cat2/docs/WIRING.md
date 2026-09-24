# Wiring

All pins below are defined in `cat2/include/config.h`; do not alter wiring based only on an old README.

| Function | Mega pin / connection |
|---|---|
| Compressor relay | D43, active LOW |
| Vent relay | D42, active LOW |
| Fan PWM | D9; polarity is controlled only by `FAN_PWM_INVERTED` |
| Gate servo | D45; 0–100% maps to 23–70 degrees |
| DS18B20 outlet temperature | A10 (Arduino digital 64; legacy sketch uses numeric 46) |
| D2 / D3 | Former pulse-flow inputs; physically disconnected and intentionally unassigned |
| I²C | SDA D20, SCL D21, start at 50 kHz |
| PCA9547 | 0x70; channels are detected at boot |
| Nextion default | Serial2: TX2 D16 → Nextion RX, RX2 D17 ← Nextion TX, 9600 |

The former NPN flow meters are not part of this firmware revision: there are no
interrupts, counters or conversion formulas for D2/D3. Do not repurpose those
pins until the physical harness and a future firmware change have both been
reviewed. If the disconnected sensor wiring is ever restored, its signal must
use a common ground and an **external 4.7–10 kΩ pull-up to Arduino 5 V**.
Never pull an Arduino input to 12 V or 24 V; keep the sensor arrow aligned with
flow direction.

If Serial1 is intentionally selected in `config.h`, use TX1 D18 → Nextion RX and RX1 D19 ← Nextion TX. Do not change Nextion baud rate unless the HMI is changed and recompiled too.

The PCA9547 may expose multiple devices at the same I²C address because only one channel is selected at a time. Exact MLX90614 and ToF channel positions are discovered and printed by diagnostics; verify them on the actual apparatus.
