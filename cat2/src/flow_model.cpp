#include <math.h>
#include "config.h"
#include "flow_model.h"
#include "pure_logic.h"

namespace {
struct SaturationCurve { float a; float k; };

static float curveFlow(const SaturationCurve &curve, float powerPercent) {
  const float power=clampValue(powerPercent,0.0f,100.0f);
  if (power<=0.0f) return 0.0f;
  const float velocity=curve.a*(1.0f-expf(-curve.k*power));
  return clampNonNegative(FLOW_MODEL_LPM_PER_MPS*velocity);
}

static FlowEstimate atDirectGate(uint8_t curveIndex, float powerPercent) {
  static const SaturationCurve output1[] = {
    {FLOW_MODEL_NEG100_OUTPUT1_A,FLOW_MODEL_NEG100_OUTPUT1_K},
    {FLOW_MODEL_NEG50_OUTPUT1_A,FLOW_MODEL_NEG50_OUTPUT1_K},
    {FLOW_MODEL_ZERO_OUTPUT1_A,FLOW_MODEL_ZERO_OUTPUT1_K}
  };
  static const SaturationCurve output2[] = {
    {FLOW_MODEL_NEG100_OUTPUT2_A,FLOW_MODEL_NEG100_OUTPUT2_K},
    {FLOW_MODEL_NEG50_OUTPUT2_A,FLOW_MODEL_NEG50_OUTPUT2_K},
    {FLOW_MODEL_ZERO_OUTPUT2_A,FLOW_MODEL_ZERO_OUTPUT2_K}
  };
  const float q1=curveFlow(output1[curveIndex],powerPercent);
  const float q2=curveFlow(output2[curveIndex],powerPercent);
  return {q1,q2,q1+q2};
}

static FlowEstimate interpolate(const FlowEstimate &low, const FlowEstimate &high, float fraction) {
  const float f=clampValue(fraction,0.0f,1.0f);
  const float q1=low.output1Lpm+(high.output1Lpm-low.output1Lpm)*f;
  const float q2=low.output2Lpm+(high.output2Lpm-low.output2Lpm)*f;
  return {q1,q2,q1+q2};
}
}

FlowEstimate estimateFlow(float appliedFanPowerPercent, int16_t appliedGatePercent) {
  const float power=clampValue(appliedFanPowerPercent,0.0f,100.0f);
  if (power<=0.0f) return {0.0f,0.0f,0.0f};
  const int16_t gate=(int16_t)clampValue((float)appliedGatePercent,-100.0f,100.0f);
  const FlowEstimate minus100=atDirectGate(0,power);
  const FlowEstimate minus50=atDirectGate(1,power);
  const FlowEstimate zero=atDirectGate(2,power);

  if (gate<=-50) return interpolate(minus100,minus50,(float)(gate+100)/50.0f);
  if (gate<=0) return interpolate(minus50,zero,(float)(gate+50)/50.0f);

  // Positive gate positions are an explicitly agreed derived geometry; they
  // are not aliases for the negative direct curves.
  const FlowEstimate minus30=interpolate(minus50,zero,0.4f);
  const FlowEstimate plus50={minus30.output2Lpm,minus30.output1Lpm,0.0f};
  const FlowEstimate plus100={minus100.output2Lpm,minus100.output1Lpm,0.0f};
  const FlowEstimate plus50Total={plus50.output1Lpm,plus50.output2Lpm,
                                  plus50.output1Lpm+plus50.output2Lpm};
  const FlowEstimate plus100Total={plus100.output1Lpm,plus100.output2Lpm,
                                   plus100.output1Lpm+plus100.output2Lpm};
  if (gate<=50) return interpolate(zero,plus50Total,(float)gate/50.0f);
  return interpolate(plus50Total,plus100Total,(float)(gate-50)/50.0f);
}

FlowModelSolveResult solveFlowModel(float targetLpm, int16_t appliedGatePercent) {
  if (!(targetLpm>0.0f)) return {0.0f,FLOW_MODEL_NONE};
  const float atMaximum=estimateFlow(100.0f,appliedGatePercent).totalLpm;
  if (targetLpm>atMaximum) return {100.0f,FLOW_MODEL_UNREACHABLE};

  float low=0.0f, high=100.0f;
  for(uint8_t iteration=0;iteration<20;++iteration) {
    const float middle=(low+high)*0.5f;
    if (estimateFlow(middle,appliedGatePercent).totalLpm<targetLpm) low=middle;
    else high=middle;
  }
  const float power=(low+high)*0.5f;
  const FlowModelStatus status=power<FLOW_MODEL_CALIBRATED_MIN_POWER_PERCENT ? FLOW_MODEL_BELOW_CALIBRATED_RANGE :
      (power>FLOW_MODEL_CALIBRATED_MAX_POWER_PERCENT ? FLOW_MODEL_EXTRAPOLATED : FLOW_MODEL_OK);
  return {power,status};
}

FlowModelStatus flowModelStatusForPower(float appliedFanPowerPercent, bool relayEnabled) {
  if (!relayEnabled || appliedFanPowerPercent<=0.0f) return FLOW_MODEL_NONE;
  if (appliedFanPowerPercent<FLOW_MODEL_CALIBRATED_MIN_POWER_PERCENT) return FLOW_MODEL_BELOW_CALIBRATED_RANGE;
  if (appliedFanPowerPercent>FLOW_MODEL_CALIBRATED_MAX_POWER_PERCENT) return FLOW_MODEL_EXTRAPOLATED;
  return FLOW_MODEL_OK;
}

const char *flowModelStatusText(FlowModelStatus status) {
  switch(status) {
    case FLOW_MODEL_OK: return "OK";
    case FLOW_MODEL_BELOW_CALIBRATED_RANGE: return "BELOW CAL RANGE";
    case FLOW_MODEL_EXTRAPOLATED: return "EXTRAPOLATED";
    case FLOW_MODEL_UNREACHABLE: return "UNREACHABLE";
    default: return "NONE";
  }
}
