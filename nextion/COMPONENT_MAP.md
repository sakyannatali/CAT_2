# Current Nextion-to-firmware map

The Nextion HMI is display and button hardware only. It never owns a real actuator value or PI setpoint. Arduino is the single source of truth for each applied value, pending edit value, editing flag and independent edit timestamp.

| Purpose | Nextion component(s) | Firmware owner |
|---|---|---|
| Compressor | `fCompStatus`, `bCompOn`, `bCompOff` | `setCompressor()` |
| Vent relay | `fVentStatus`, `bVentOn`, `bVentOff` | `setVentEnabled()` |
| Mode | `fMode`, `fModeStatus`, `bAuto`, `bManual` | `setFlowControlMode()` |
| Manual fan edit | `bVentMinus`, `tVentSet`, `bVentPlus`, `bVentApply` | `ventPowerEdit`, `applyVentPowerEdit()` |
| Gate edit | `bGateMinus`, `tGateSet`, `bGatePlus`, `bGateApply` | `gateEdit`, `applyGateEdit()` |
| Flow-setpoint edit | `bFlowMinus`, `tFlowSet`, `bFlowPlus`, `bFlowApply` | `flowSetpointEdit`, `applyFlowSetpointEdit()` |
| Applied PI setpoint/output | `fSetpoint`, `fPiOutput`, `fError` | `flowSetpointLpm`, PI controller |
| Timer | `fTimer`, `bTimerStart`, `bTimerReset` | `timer_service.cpp` |
| Temperatures | `fTIn`, `fTSkin1`, `fTSkin2` | `temperature_sensors.cpp` |
| Flow display | `fVolume1`, `fVolume2` | `flow_meter.cpp` |
| Distances | `fLSkin1`, `fLSkin2` | `tof_sensors.cpp` |

`+`/`-` sends one trigger to Arduino. Arduino changes only the pending value,
updates the adjacent Text field, and restarts that parameter's own five-second
timeout. Fan changes by 1%; gate changes by 10% in its signed `-100…+100%`
command scale; flow setpoint changes by 5 L/min (snapped to 0…100 L/min).
Apply commits pending to applied. Five seconds without Apply cancels the
pending edit and restores the Text field to the applied value. The signed gate
command maps to the legacy physical scale as `(command + 100) / 2`, so `0%`
is the old midpoint.

There are no sliders, hidden Number components, `.val` reads, or `get` commands for fan power, gate or flow setpoint. In AUTO, fan Apply only stores the future MANUAL fan value; PWM remains PI-controlled by the sum of both valid flow meters.
