#pragma once
#include <Arduino.h>

enum FlowControlMode : uint8_t { FLOW_MANUAL, FLOW_AUTO };
enum FlowFeedbackSource : uint8_t { FLOW_SENSOR_1, FLOW_SENSOR_2, FLOW_AVERAGE_OF_VALID };
struct ValueState { float value; bool valid; uint32_t updatedMs; };
struct FlowState {
  uint32_t totalPulses, lastPulsePeriodUs, lastPulseAgeMs;
  float frequencyHz, massFlowKgH, filteredMassFlowKgH;
  bool valid, stale, calibrated;
};
struct TofState {
  uint8_t channel, modelId, revisionId;
  uint16_t distanceMm;
  bool present, supportedModel, initialized, timeout, valid;
  uint32_t updatedMs;
};
struct AppState {
  bool compressorOn, ventRelayOn;
  float requestedFanPowerPercent, appliedFanPowerPercent;
  float gatePercent; uint8_t gateAngle;
  FlowControlMode flowControlMode; float flowSetpointKgH;
  bool timerRunning; uint32_t timerElapsedMs;
  ValueState outletTemperature, skinTemperature[2];
  FlowState flow[2]; TofState tof[2];
  float piOutputPercent, piError, piP, piI;
  bool autoBlocked; const char *autoBlockReason;
};
extern AppState app;
void appStateBegin();
void setCompressor(bool on);
void setVentEnabled(bool on);
void setFanPowerPercent(float percent);
void setGatePercent(float percent);
bool setFlowControlMode(FlowControlMode mode);
void setFlowSetpointKgH(float kgH);
void setFlowFeedbackSource(FlowFeedbackSource source);
FlowFeedbackSource getFlowFeedbackSource();
