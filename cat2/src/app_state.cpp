#include "app_state.h"
#include "config.h"
#include "actuators.h"
#include "model_control.h"

AppState app;

static uint8_t gateAngleFor(float physicalPercent) {
  return (uint8_t)(GATE_ANGLE_MIN+(physicalPercent*(GATE_ANGLE_MAX-GATE_ANGLE_MIN)/100.0f)+0.5f);
}
static void applyActualFan(float commandPercent) {
  const float command=app.ventRelayOn ? clampValue(commandPercent,0.0f,100.0f) : 0.0f;
  const uint8_t pwm=actuatorsApplyFan(command);
  const bool changed=pwm!=app.fanPwmRaw;
  app.appliedFanPowerPercent=command;
  app.fanPwmRaw=pwm;
  if(changed) {
    Serial.print(F("Fan command: ")); Serial.print(command,1);
    Serial.print(F("% -> PWM ")); Serial.println(pwm);
  }
}
static void applyGatePosition(float signedPercent) {
  app.gateCommandPercent=clampValue(signedPercent,GATE_COMMAND_MIN_PERCENT,GATE_COMMAND_MAX_PERCENT);
  app.gatePercent=signedGateToPhysicalPercent(app.gateCommandPercent);
  app.gateAngle=gateAngleFor(app.gatePercent);
  actuatorsApplyGate(app.gateAngle);
}
static void adoptCurrentFanAsManual() {
  app.requestedFanPowerPercent=manualPowerAfterAutoFallback(app.appliedFanPowerPercent);
  pendingApplyInitialize(app.ventPowerEdit,app.requestedFanPowerPercent);
}

void appStateBegin() {
  app.compressorOn=false;
  app.ventRelayOn=false;
  app.requestedFanPowerPercent=0;
  app.appliedFanPowerPercent=0;
  app.fanPwmRaw=FAN_PWM_INVERTED ? 255 : 0;
  app.gateCommandPercent=0;
  app.gatePercent=50;
  app.gateAngle=gateAngleFor(app.gatePercent);
  app.flowControlMode=FLOW_MANUAL;
  pendingApplyInitialize(app.ventPowerEdit,0);
  pendingApplyInitialize(app.gateEdit,0);
  flowSetpointInitialize(app.flowSetpoint,false,0);
  app.timerRunning=false;
  app.timerElapsedMs=0;
  temperatureStateInitialize(app.outletTemperature);
  for(uint8_t i=0;i<2;++i) {
    temperatureStateInitialize(app.skinTemperature[i]);
    app.tof[i]={255,0,0,0,false,false,false,false,false,0};
  }
  app.modelFlow={0,0,0};
  app.modelTargetFanPowerPercent=0;
  app.modelStatus=FLOW_MODEL_NONE;
}

void setCompressor(bool on) { app.compressorOn=on; actuatorsApplyCompressor(on); }
void setVentEnabled(bool on) {
  app.ventRelayOn=on;
  actuatorsApplyVent(on);
  applyActualFan(activeFanCommandPercent(app.flowControlMode==FLOW_AUTO,app.requestedFanPowerPercent,app.modelTargetFanPowerPercent));
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
}
void setFanPowerPercent(float percent) {
  app.requestedFanPowerPercent=clampValue(percent,0.0f,100.0f);
  pendingApplyInitialize(app.ventPowerEdit,app.requestedFanPowerPercent);
  if(app.flowControlMode==FLOW_MANUAL) applyActualFan(app.requestedFanPowerPercent);
}
void setAutomaticFanPowerPercent(float percent) { applyActualFan(percent); }
void setGateCommandPercent(int16_t signedPercent) {
  const float command=clampValue((float)signedPercent,GATE_COMMAND_MIN_PERCENT,GATE_COMMAND_MAX_PERCENT);
  pendingApplyInitialize(app.gateEdit,command);
  applyGatePosition(command);
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
}
bool setFlowControlMode(FlowControlMode mode) {
  if(mode==FLOW_MANUAL && app.flowControlMode==FLOW_AUTO) adoptCurrentFanAsManual();
  app.flowControlMode=mode;
  if(mode==FLOW_AUTO) modelControlRequestAutoUpdate();
  else applyActualFan(app.requestedFanPowerPercent);
  return true;
}
bool setFlowSetpointLpm(float lpm) {
  if(lpm==0.0f) { setFlowSetpointNone(); return true; }
  if(!flowSetpointValueAllowed(lpm,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM)) return false;
  flowSetpointInitialize(app.flowSetpoint,true,lpm);
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
  return true;
}
void setFlowSetpointNone() {
  flowSetpointInitialize(app.flowSetpoint,false,0);
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
}

void editVentPower(int8_t direction,uint32_t now) {
  pendingApplyAdjust(app.ventPowerEdit,direction,1.0f,0.0f,100.0f,now);
}
void editGate(int8_t direction,uint32_t now) {
  pendingApplyAdjust(app.gateEdit,direction,GATE_COMMAND_EDIT_STEP_PERCENT,GATE_COMMAND_MIN_PERCENT,GATE_COMMAND_MAX_PERCENT,now);
}
void editFlowSetpoint(int8_t direction,uint32_t now) {
  flowSetpointAdjust(app.flowSetpoint,direction,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,FLOW_SETPOINT_EDIT_STEP_LPM,now);
}
bool applyVentPowerEdit() {
  if(!pendingApplyCommit(app.ventPowerEdit)) return false;
  app.requestedFanPowerPercent=app.ventPowerEdit.applied;
  if(app.flowControlMode==FLOW_MANUAL) applyActualFan(app.requestedFanPowerPercent);
  else Serial.println(F("Manual fan value stored; AUTO controls PWM."));
  return true;
}
bool applyGateEdit() {
  if(!pendingApplyCommit(app.gateEdit)) return false;
  applyGatePosition(app.gateEdit.applied);
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
  return true;
}
bool applyFlowSetpointEdit() {
  if(!flowSetpointCommit(app.flowSetpoint)) return false;
  if(app.flowControlMode==FLOW_AUTO) modelControlRequestAutoUpdate();
  return true;
}
void appStateService(uint32_t now) {
  pendingApplyTimedOut(app.ventPowerEdit,now,EDIT_APPLY_TIMEOUT_MS);
  pendingApplyTimedOut(app.gateEdit,now,EDIT_APPLY_TIMEOUT_MS);
  flowSetpointTimedOut(app.flowSetpoint,now,EDIT_APPLY_TIMEOUT_MS);
}
