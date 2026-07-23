# Required Nextion Editor changes

`ui.HMI`, `Basic.zi`, and `Russian.zi` are binary Nextion assets. No editable project representation or Nextion Editor was available, so they were deliberately not text-edited and no fictitious `.tft` was created.

Open `nextion/ui.HMI` in Nextion Editor. Retain the current baud rate at 9600. Convert the listed display fields to **Text** components (the firmware writes `<name>.txt`, never rounded numeric `.val`). Use a font containing the required Cyrillic glyphs; `Russian.zi` is present in the repository.

| Visible purpose | Component name | Required text / range |
|---|---|---|
| Outlet temperature | `fTIn` | Relabel “Температура на выходе”; text |
| Flow 1 / 2 | `fVolume1`, `fVolume2` | text; Hz before calibration, кг/ч after |
| Skin temperature 1 / 2 | `fTSkin1`, `fTSkin2` | text |
| Distance 1 / 2 | `fLSkin1`, `fLSkin2` | text |
| Compressor / vent state | `fCompStatus`, `fVentStatus` | new Text |
| Applied fan power / gate | `fVentPower`, `fGate` | new Text |
| Timer / mode / setpoint / PI output | `fTimer`, `fMode`, `fSetpoint`, `fPiOutput` | new Text |
| Error | `fError` | new Text |
| Fan slider / gate slider | `sVentSpeed`, `sGate` | keep numeric, range 0–100 |
| AUTO controls | `bManual`, `bAuto`, `nFlowSetpoint` | new buttons; Number range 0–65535 |

Remove the “Холодильная машина” section and its values. In the air circuit remove “temperature at inlet”; retain only outlet temperature, both flows, skin temperatures 1/2, and distances 1/2. Add the new status, setpoint, PI-output, timer, mode, and error text fields.

For **Touch Release** events, preserve the existing protocol exactly:

| Action | Nextion event command |
|---|---|
| Compressor on / off | `printh 23 02 54 00` / `printh 23 02 54 01` |
| Vent on / off | `printh 23 02 54 02` / `printh 23 02 54 03` |
| Fan slider changed | `printh 23 02 54 04` |
| Gate slider changed | `printh 23 02 54 05` |
| Timer start/pause | `printh 23 02 54 06` |
| Timer reset | `printh 23 02 54 07` |
| MANUAL / AUTO | `printh 23 02 54 08` / `printh 23 02 54 09` |
| Flow setpoint changed | `printh 23 02 54 0A` |

The firmware requests `sVentSpeed.val` or `sGate.val` itself after the matching trigger, so the current button/slider protocol remains compatible and performs no direct PWM, relay, or servo operation in the UI layer.

Compile the HMI in Nextion Editor, verify all component names, then use the editor's Compile output `.tft`. Do not copy it to hardware or flash the display without separate authorisation.
