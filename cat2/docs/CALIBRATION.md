# Flow calibration status

The pulse-meter calibration path has been retired because the D2/D3 meters are
physically disconnected. Current firmware uses the fixed experimental
open-loop curves documented in [FLOW_MODEL.md](FLOW_MODEL.md), not a sensor
calibration table. New measured data should be reviewed before changing the
six `A`/`k` curve pairs in `cat2/include/config.h`.
