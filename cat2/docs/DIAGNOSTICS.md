# Diagnostics

Use the 9600-baud monitor. Commands are bounded to 95 characters.

| Command | Result |
|---|---|
| `status` | All actuators, temperatures, flows, ToF, timer and PI state |
| `flow`, `flow raw`, `flow reset` | Counters, Hz, period, age, kg/h where calibrated, stale flag |
| `i2c scan`, `i2c mux` | Raw bus or PCA9547 channel scan |
| `tof scan`, `tof status`, `tof read` | Mux/0x29/model/revision/init/measurement diagnosis |
| `timer start|pause|reset|status` | Arduino timer state |
| `control manual|auto` | Select control mode |
| `control setpoint <kg/h>` | Set AUTO target |
| `control kp <value>`, `control ti <s>` | Change PI parameters for this boot |
| `control source 1|2|avg`, `control status` | Feedback source/status |
| `debug off|sensors|all` | Controlled periodic diagnostics |

ToF output identifies: no hub ACK, failed channel selection, no 0x29 device, exact model/revision, unsupported model, init failure, timeout, invalid distance, and stale measurement. A non-`0xEE` model is not treated as VL53L0X.
