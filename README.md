# CAT 2 firmware

The single current firmware is the PlatformIO project in [`cat2/`](cat2).
Open [`cat2.code-workspace`](cat2.code-workspace) in VS Code. `sketches/main`
is retained solely as a read-only legacy reference; it must not become a second
development copy.

## Build and use

```powershell
pio run -d cat2
pio run -d cat2 -t upload
pio device monitor -d cat2
```

The monitor and firmware use 9600 baud. The default environment is
`megaatmega2560` (Arduino Mega 2560); no COM port is fixed. Upload is a
manual action and is not run by this project work.

## Safety and current hardware assumptions

- Relays are active-low and initialise OFF. AUTO is OFF after reboot.
- DS18B20 remains on the working `A10` / Mega digital 64 connection (the
  legacy numeric sketch also calls it `46`). Do not change its harness from an
  old README alone.
- Nextion stays on `Serial2`: Mega TX2 D16 → Nextion RX and Mega RX2 D17 ←
  Nextion TX, at 9600 baud. Set `NEXTION_USE_SERIAL1` in
  [`cat2/include/config.h`](cat2/include/config.h) only when moving to the
  intentionally documented D18/D19 wiring and recompiling the HMI.
- `nextion/ui_final.HMI` is manually maintained. Firmware neither changes,
  compiles, nor uploads an HMI/TFT file.
- Neither Arduino firmware nor a Nextion TFT has been uploaded to hardware.

## Experimental model flow and AUTO

The former D2/D3 pulse flow meters are physically disconnected. The firmware
does not attach flow interrupts, read pulse counters, calculate Hz, or apply
density/temperature correction. D2 and D3 are deliberately left unassigned
and must not be casually repurposed.

`fVolume1` and `fVolume2` now show open-loop calculated estimates in L/min.
They use only actual applied fan power (zero while the vent relay is off) and
the applied signed gate value. They are **not measured flow**.

AUTO is a feed-forward inverse of the agreed experimental model, not PI
feedback. It does not toggle the vent relay. Its committed target scale is
`NONE`, `30`, `35`, …, `500 L/min`; `NONE` commands model PWM `0%` but leaves
the relay state unchanged. The experimental model is qualified only from 5 to
60% fan power. Results below this range are marked `BELOW CAL RANGE`, values
above it are `EXTRAPOLATED`, and an impossible demand is `UNREACHABLE` at
100%. See [`cat2/docs/FLOW_MODEL.md`](cat2/docs/FLOW_MODEL.md).

The AUTO target is always total flow: `Qtotal = Q1 + Q2`. `500 L/min` is a
user-input ceiling, not a clamp on either calculated output or a promise that
the model can attain that total at the current gate position.

## Nextion edit/apply behavior

Fan power, gate and flow target use `-`, `+` and Apply. Each press changes
only an Arduino pending value: 1% fan power, 10% signed gate command, and one
flow-target step. Gate is `-100…+100%`; zero maps to the old physical midpoint
(50%). Flow target cycles `NONE → 30 → 35 … → 500` and back. Apply commits the
pending value. Each control has an independent non-blocking 5-second timeout
from its last edit; expiry restores the applied value.

In AUTO, `tVentSet` shows `AUTO` and fan Apply cannot override model PWM. On
AUTO→MANUAL, Arduino adopts the actual applied model command as the new manual
value, so there is no PWM step. `fPiOutput` retains its legacy HMI name but now
means **applied AUTO model power**.

## Temperature fault policy

DS18B20 and both GY-906 channels retain a last-good value, timestamp and
failure count. Three failures or a 2.5-second age marks a channel stale;
recovery is attempted at most once per second. A stale temperature remains a
diagnostic condition but does not stop model AUTO, because this model has no
temperature or density feedback.

## Documentation

- [`cat2/docs/WIRING.md`](cat2/docs/WIRING.md)
- [`cat2/docs/FLOW_MODEL.md`](cat2/docs/FLOW_MODEL.md)
- [`cat2/docs/PI_CONTROL.md`](cat2/docs/PI_CONTROL.md)
- [`cat2/docs/DIAGNOSTICS.md`](cat2/docs/DIAGNOSTICS.md)
- [`cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md`](cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md)
