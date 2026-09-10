#pragma once
#include <Arduino.h>
#include "pure_logic.h"

enum FlowControlMode : uint8_t { FLOW_MANUAL, FLOW_AUTO };
struct ValueState { float value; bool valid; uint32_t updatedMs; };
struct FlowState {
  uint32_t totalPulses, lastPulsePeriodUs, lastPulseAgeMs;
  float frequencyHz, instantFlowLpm, filteredFlowLpm, displayFlowLpm;
  float densityCorrection, outletTemperatureUsedC;
  bool valid, stale, zeroFlow, conversionConfigured, densityValid;
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
  float gatePercent, gateCommandPercent; uint8_t gateAngle;
  FlowControlMode flowControlMode; float flowSetpointLpm;
  PendingApplyState ventPowerEdit, gateEdit, flowSetpointEdit;
  bool timerRunning; uint32_t timerElapsedMs;
  ValueState outletTemperature, skinTemperature[2];
  FlowState flow[2]; TofState tof[2];
  float piOutputPercent, piRawOutputPercent, piError, piFlowErrorLpm, piP, piI;
  uint8_t fanPwmRaw;
  bool autoBlocked; const char *autoBlockReason;
};
extern AppState app;
void appStateBegin();
void setCompressor(bool on);
void setVentEnabled(bool on);
void setFanPowerPercent(float percent);
void setAutomaticFanPowerPercent(float percent);
void setGateCommandPercent(int16_t signedPercent);
bool setFlowControlMode(FlowControlMode mode);
bool setFlowSetpointLpm(float lpm);
void editVentPower(int8_t direction, uint32_t now);
void editGate(int8_t direction, uint32_t now);
void editFlowSetpoint(int8_t direction, uint32_t now);
bool applyVentPowerEdit();
bool applyGateEdit();
bool applyFlowSetpointEdit();
void appStateService(uint32_t now);
void stopAutoForSafety(const char *reason);
