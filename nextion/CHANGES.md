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
| `0x0D` | gate -1% | `printh 23 02 54 0D` |
| `0x0E` | gate +1% | `printh 23 02 54 0E` |
| `0x0F` | flow -1 L/min | `printh 23 02 54 0F` |
| `0x10` | flow +1 L/min | `printh 23 02 54 10` |

Do not add `cov`, `+=`, a slider or a Number component for these three values. The HMI must not change Text values itself. On boot, Arduino pushes every Text field so retained HMI values are never treated as state. This repository does not compile, produce or upload a `.tft` file.
