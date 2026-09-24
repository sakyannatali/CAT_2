#pragma once

#include <stdint.h>

// Open-loop experimental estimate, not a measurement. Values are L/min.
enum FlowModelStatus : uint8_t {
  FLOW_MODEL_NONE,
  FLOW_MODEL_OK,
  FLOW_MODEL_BELOW_CALIBRATED_RANGE,
  FLOW_MODEL_EXTRAPOLATED,
  FLOW_MODEL_UNREACHABLE
};

struct FlowEstimate {
  float output1Lpm;
  float output2Lpm;
  float totalLpm;
};

struct FlowModelSolveResult {
  float fanPowerPercent;
  FlowModelStatus status;
};

// Uses applied fan power and the applied signed gate command only.
FlowEstimate estimateFlow(float appliedFanPowerPercent, int16_t appliedGatePercent);
FlowModelSolveResult solveFlowModel(float targetLpm, int16_t appliedGatePercent);
FlowModelStatus flowModelStatusForPower(float appliedFanPowerPercent, bool relayEnabled);
const char *flowModelStatusText(FlowModelStatus status);
