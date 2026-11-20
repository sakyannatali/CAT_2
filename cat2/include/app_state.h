#pragma once
#include <Arduino.h>
#include "pure_logic.h"
#include "flow_model.h"

enum FlowControlMode : uint8_t { FLOW_MANUAL, FLOW_AUTO };
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
  FlowControlMode flowControlMode;
  PendingApplyState ventPowerEdit, gateEdit;
  FlowSetpointState flowSetpoint;
  bool timerRunning; uint32_t timerElapsedMs;
  TemperatureFaultState outletTemperature, skinTemperature[2];
  TofState tof[2];
  FlowEstimate modelFlow;
  float modelTargetFanPowerPercent;
  FlowModelStatus modelStatus;
  uint8_t fanPwmRaw;
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
void setFlowSetpointNone();
void editVentPower(int8_t direction, uint32_t now);
void editGate(int8_t direction, uint32_t now);
void editFlowSetpoint(int8_t direction, uint32_t now);
bool applyVentPowerEdit();
bool applyGateEdit();
bool applyFlowSetpointEdit();
void appStateService(uint32_t now);
