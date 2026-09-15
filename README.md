# CAT 2 firmware

The current, single source of firmware is the PlatformIO project in [`cat2/`](cat2). Open `cat2.code-workspace` in VS Code. `sketches/main` is retained only as a read-only legacy reference; do not develop a second firmware copy there.

## Build and use

```powershell
pio run -d cat2
pio run -d cat2 -t upload
pio device monitor -d cat2
```

The monitor and firmware use 9600 baud. `upload` is intentionally not run by this migration.

The default target is `megaatmega2560` (Arduino Mega 2560). No COM port is fixed in the project.

## Safety and current status

- Relays are active-low and initialise OFF; AUTO flow control is OFF after reboot.
- The DS18B20 pin is kept at the working value from `sketches/main`: `A10` (Mega digital 64 / physical legacy value 46). Older documentation calling it “A10” is not a reason to alter the harness.
- Nextion defaults to `Serial2`: Mega TX2=16, RX2=17, 9600 baud. It was not changed to the README's obsolete Serial1 wiring. To intentionally move it, set `NEXTION_USE_SERIAL1` to `1` in `cat2/include/config.h`, then wire Mega TX1=18 → Nextion RX and Mega RX1=19 ← Nextion TX.
- `nextion/ui_final.HMI` is manually maintained and is not modified by firmware work. The HMI sends only button triggers; Arduino owns applied/pending values and restores un-applied edits after five seconds. The current protocol is in [`nextion/CHANGES.md`](nextion/CHANGES.md).
- Neither Arduino firmware nor a Nextion `.tft` has been uploaded to hardware.
- Hardware `Wire` uses a 50 ms timeout and resets TWI after a timeout. A failed I²C temperature transaction returns control to the loop; it never resets the MCU.

## Flow conversion and PI control

The experimental flow conversions are built into the current firmware. The UI
uses density-corrected L/min and refreshes each flow display from its
non-blocking 5-second moving average. Raw Hz remains available only through
`flow raw` diagnostics; AUTO is no longer blocked by the obsolete
`FLOW_CALIBRATED` flag.

AUTO uses the total of both filtered flow readings: `Flow 1 + Flow 2`. Both
meters must be valid; loss of either one exits AUTO to MANUAL while retaining
the current safe fan command. The initial PI parameters are Kp=0.5
(normalised-percent error) and Ti=100 s. See
[`cat2/docs/FLOW_CALIBRATION.md`](cat2/docs/FLOW_CALIBRATION.md) and
[`cat2/docs/PI_CONTROL.md`](cat2/docs/PI_CONTROL.md).

The PI limiter is asymmetric: a rise is limited to 10 percentage points per
second and a fall to 20 percentage points per second. This keeps acceleration
measured while letting the controller reduce excessive flow faster.

## Nextion edit/apply behavior

Fan power, gate position and flow setpoint use `-`, `+` and Apply buttons.
Each press changes only a pending Arduino value: 1% for fan power, 10% for the
signed gate command, and 5 L/min for the flow setpoint. Gate command is
`-100…+100%`; `0%` maps to the old physical midpoint (50%). Apply commits the
value. Each parameter has an independent non-blocking five-second timeout
measured from its last press; expiry discards the pending value and restores
the Text field. In AUTO, a fan Apply cannot override PI PWM; switching back to
MANUAL adopts the current actual PI output instead.

In AUTO, `tVentSet` shows exactly `AUTO`, not a manual percentage. On
AUTO→MANUAL, Arduino adopts the current actual PI output as both the applied
and pending manual setting, so the field returns to that percentage without a
fan step.

## Temperature fault policy

DS18B20 and both GY-906 channels keep their last-good value, timestamp and
consecutive-failure count. One or two failed reads retain the last-good value;
three failures or an age of 2.5 seconds makes a channel stale. Failed channels
are retried at most once per second. A stale outlet temperature invalidates
density correction and safely changes AUTO to MANUAL while preserving the
current fan command. MANUAL, Nextion, flow counting and other controls keep
running.

## Documentation

- [`cat2/docs/WIRING.md`](cat2/docs/WIRING.md)
- [`cat2/docs/DIAGNOSTICS.md`](cat2/docs/DIAGNOSTICS.md)
- [`cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md`](cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md)
