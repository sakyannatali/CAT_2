# Nextion-to-firmware map

| Purpose | Nextion component | Firmware owner |
|---|---|---|
| Compressor state/action | `fCompStatus`, trigger 0/1 | `setCompressor()` / `app_state.cpp` |
| Vent state/action | `fVentStatus`, trigger 2/3 | `setVentEnabled()` / `app_state.cpp` |
| Fan slider/output | `sVentSpeed`, `fVentPower`, trigger 4 | `setFanPowerPercent()` / `actuators.cpp` |
| Gate slider/output | `sGate`, `fGate`, trigger 5 | `setGatePercent()` / `actuators.cpp` |
| Timer | `fTimer`, trigger 6/7 | `timer_service.cpp` |
| Temperatures | `fTIn`, `fTSkin1`, `fTSkin2` | `temperature_sensors.cpp` |
| Flows | `fVolume1`, `fVolume2` | `flow_meter.cpp` |
| Distances | `fLSkin1`, `fLSkin2` | `tof_sensors.cpp` |
| AUTO values/errors | `fMode`, `fSetpoint`, `fPiOutput`, `fError` | `pi_controller.cpp` |
| AUTO controls | `bManual`, `bAuto`, `nFlowSetpoint`, triggers 8/9/10 | `setFlowControlMode()` / `setFlowSetpointKgH()` |
