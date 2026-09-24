# Current Nextion-to-firmware map

The Nextion is display and button hardware only. Arduino owns every applied
value, pending edit, timestamp and actuator command. `ui_final.HMI` was not
edited automatically.

| Purpose | Nextion component(s) | Firmware owner |
|---|---|---|
| Compressor | `fCompStatus`, `bCompOn`, `bCompOff` | `setCompressor()` |
| Vent relay | `fVentStatus`, `bVentOn`, `bVentOff` | `setVentEnabled()` |
| Mode | `fMode`, `fModeStatus`, `bAuto`, `bManual` | `setFlowControlMode()` |
| Manual/AUTO fan | `bVentMinus`, `tVentSet`, `bVentPlus`, `bVentApply` | `ventPowerEdit`, `applyVentPowerEdit()` |
| Gate edit | `bGateMinus`, `tGateSet`, `bGatePlus`, `bGateApply` | `gateEdit`, `applyGateEdit()` |
| Flow-target edit | `bFlowMinus`, `tFlowSet`, `bFlowPlus`, `bFlowApply` | `flowSetpoint`, `applyFlowSetpointEdit()` |
| Applied target / AUTO power / warning | `fSetpoint`, `fPiOutput`, `fError` | `flowSetpoint`, `modelTargetFanPowerPercent`, `modelStatus` |
| Timer | `fTimer`, `bTimerStart`, `bTimerReset` | `timer_service.cpp` |
| Temperatures | `fTIn`, `fTSkin1`, `fTSkin2` | `temperature_sensors.cpp` |
| Calculated output flows | `fVolume1`, `fVolume2` | `flow_model.cpp` / `model_control.cpp` |
| Distances | `fLSkin1`, `fLSkin2` | `tof_sensors.cpp` |

The existing event protocol stays unchanged. Fan steps by 1%; signed gate steps
by 10%. Flow target steps `NONE → 30 → 35 … → 100`, and minus from 30 returns
to `NONE`. Apply commits; each field independently cancels after 5 seconds.
`fSetpoint` displays applied `NONE` or the applied L/min target. While an edit
is pending, `tFlowSet` alone shows its pending `NONE` or target value.

No slider, Number component, `.val` read or `get` command is needed. In AUTO,
fan Apply cannot replace model PWM, and `tVentSet` shows `AUTO`. `fPiOutput`
is a legacy component name: it now means the actual applied model power, not a
PI result. The HMI label should be changed manually to “AUTO power” in Nextion
Editor when the next approved HMI update is made.
