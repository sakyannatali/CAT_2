# Diagnostics

Use the 9600-baud monitor. Commands are bounded to 95 characters.

| Command | Result |
|---|---|
| `status` | Actuators, temperatures, both flows, ToF, timer and PI state |
| `flow` / `flow raw` | Pulses, raw Hz, pulse age, instant/filtered/display L/min, temperature, density factor, zero-flow and validity |
| `flow density` | `rho24`, outlet temperature used, `rhoOut`, correction and validity |
| `flow reset` | Reset counters and 5-second filters |
| `control manual\|auto` | Choose the control mode |
| `control setpoint <L/min>` | Set target in the inclusive range 0…100 L/min |
| `control kp <value>`, `control ti <s>` | Change PI parameters for this boot |
| `control source 1\|2\|avg`, `control status` | Feedback source/status |
| `timer start\|pause\|reset\|status` | Arduino timer state |
| `i2c scan`, `i2c mux` | Raw bus or PCA9547 channel scan |
| `tof on\|off\|scan\|status\|read` | ToF diagnosis |
| `debug off\|sensors\|all` | Controlled periodic diagnostics |

`flow raw` leaves the user interface untouched; raw Hz is diagnostic-only.
The normal UI receives only the 5-second filtered L/min display value.
