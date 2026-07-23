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
- The HMI source is a binary Nextion project and was not modified by text tooling. Firmware support and exact editor changes are in [`nextion/CHANGES.md`](nextion/CHANGES.md).
- Neither Arduino firmware nor a Nextion `.tft` has been uploaded to hardware.

## Calibration and PI control

The two calibration tables are in [`cat2/include/config.h`](cat2/include/config.h), `FLOW1_CALIBRATION` and `FLOW2_CALIBRATION`. Leave `FLOW_CALIBRATED=false` until measured points have been inserted and pass validation. In that state the display reports Hz and AUTO is deliberately blocked.

The initial PI parameters are Kp=0.5 (normalised error) and Ti=100 s. See [`cat2/docs/CALIBRATION.md`](cat2/docs/CALIBRATION.md) and [`cat2/docs/PI_CONTROL.md`](cat2/docs/PI_CONTROL.md).

## Documentation

- [`cat2/docs/WIRING.md`](cat2/docs/WIRING.md)
- [`cat2/docs/DIAGNOSTICS.md`](cat2/docs/DIAGNOSTICS.md)
- [`cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md`](cat2/docs/MIGRATION_FROM_ARDUINO_IDE.md)
