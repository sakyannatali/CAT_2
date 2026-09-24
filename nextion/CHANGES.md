# Current HMI protocol

`ui_final.HMI` is maintained manually in Nextion Editor and is intentionally not edited by this firmware task. The firmware expects the existing Text and button component names listed in [COMPONENT_MAP.md](COMPONENT_MAP.md).

Touch Release events must send exactly these packets:

| Trigger | Action | `printh` |
|---:|---|---|
| `0x00` | compressor ON | `printh 23 02 54 00` |
| `0x01` | compressor OFF | `printh 23 02 54 01` |
| `0x02` | vent ON | `printh 23 02 54 02` |
| `0x03` | vent OFF | `printh 23 02 54 03` |
| `0x04` | apply fan pending value | `printh 23 02 54 04` |
| `0x05` | apply gate pending value | `printh 23 02 54 05` |
| `0x06` | timer start/pause | `printh 23 02 54 06` |
| `0x07` | timer reset | `printh 23 02 54 07` |
| `0x08` | MANUAL | `printh 23 02 54 08` |
| `0x09` | AUTO | `printh 23 02 54 09` |
| `0x0A` | apply flow pending value | `printh 23 02 54 0A` |
| `0x0B` | fan -1% | `printh 23 02 54 0B` |
| `0x0C` | fan +1% | `printh 23 02 54 0C` |
| `0x0D` | gate -10% (signed command) | `printh 23 02 54 0D` |
| `0x0E` | gate +10% (signed command) | `printh 23 02 54 0E` |
| `0x0F` | flow target − one step (`30 → NONE`) | `printh 23 02 54 0F` |
| `0x10` | flow target + one step (`NONE → 30`) | `printh 23 02 54 10` |

Do not add `cov`, `+=`, a slider or a Number component for these three values. The HMI must not change Text values itself. On boot, Arduino pushes every Text field so retained HMI values are never treated as state. This repository does not compile, produce or upload a `.tft` file.

`tVentSet` is firmware-owned: it displays a pending/applied percentage in
MANUAL and exactly `AUTO` while feed-forward model AUTO is active. The same
event protocol remains in use; no `.val` request is permitted.

## Pending manual HMI text update

The binary HMI was not modified automatically. When an approved Nextion Editor
session is available, keep all component names and Touch Release `printh`
packets above, then change visible labels only:

- rename any “PI output” caption next to `fPiOutput` to **AUTO power**;
- label `fVolume1` and `fVolume2` as **Calculated output 1/2, L/min** (not
  measured flow);
- label `fSetpoint` as **Applied model target** and ensure `NONE` fits;
- make the warning area next to `fError` wide enough for `BELOW CAL RANGE`,
  `EXTRAPOLATED`, and `UNREACHABLE`.

Compile the unchanged-protocol project in Nextion Editor, review the generated
`.tft`, then load it only after separate explicit approval. This repository
does not create or upload a `.tft` file.
