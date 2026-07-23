# Wiring

All pins below are defined in `cat2/include/config.h`; do not alter wiring based only on an old README.

| Function | Mega pin / connection |
|---|---|
| Compressor relay | D43, active LOW |
| Vent relay | D42, active LOW |
| Fan PWM | D9; polarity is controlled only by `FAN_PWM_INVERTED` |
| Gate servo | D45; 0–100% maps to 23–70 degrees |
| DS18B20 outlet temperature | A10 (Arduino digital 64; legacy sketch uses numeric 46) |
| Flow meter 1 | D2 interrupt |
| Flow meter 2 | D3 interrupt |
| I²C | SDA D20, SCL D21, start at 50 kHz |
| PCA9547 | 0x70; channels are detected at boot |
| Nextion default | Serial2: TX2 D16 → Nextion RX, RX2 D17 ← Nextion TX, 9600 |

For each NPN flow meter, connect sensor ground to Arduino ground and add an **external 4.7–10 kΩ pull-up to Arduino 5 V** on the signal. Never pull an Arduino input to 12 V or 24 V. The sensor arrow must match actual flow direction. The firmware counts the falling edge by default and rejects pulses closer than the configured minimum period.

If Serial1 is intentionally selected in `config.h`, use TX1 D18 → Nextion RX and RX1 D19 ← Nextion TX. Do not change Nextion baud rate unless the HMI is changed and recompiled too.

The PCA9547 may expose multiple devices at the same I²C address because only one channel is selected at a time. Exact MLX90614 and ToF channel positions are discovered and printed by diagnostics; verify them on the actual apparatus.
