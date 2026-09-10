#include "app_state.h"
#include "config.h"
#include "actuators.h"
#include "flow_meter.h"
#include "pi_controller.h"

AppState app;

static bool autoFeedbackAvailable() {
  const FlowState &a=app.flow[0], &b=app.flow[1];
  return a.valid&&a.conversionConfigured&&!isnan(a.filteredFlowLpm) &&
         b.valid&&b.conversionConfigured&&!isnan(b.filteredFlowLpm);
}
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
  app.requestedFanPowerPercent=app.appliedFanPowerPercent;
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
  app.flowSetpointLpm=0;
  pendingApplyInitialize(app.ventPowerEdit,0);
  pendingApplyInitialize(app.gateEdit,0);
  pendingApplyInitialize(app.flowSetpointEdit,0);
  app.timerRunning=false;
  app.timerElapsedMs=0;
  app.outletTemperature={0,false,0};
  for(uint8_t i=0;i<2;++i) {
    app.skinTemperature[i]={0,false,0};
    app.flow[i]={0,0,0,0,NAN,NAN,NAN,NAN,NAN,false,true,false,FLOW_CONVERSION_CONFIGURED,false};
    app.tof[i]={255,0,0,0,false,false,false,false,false,0};
  }
  app.piOutputPercent=0;
  app.piRawOutputPercent=0;
  app.piError=app.piFlowErrorLpm=app.piP=app.piI=0;
  app.autoBlocked=false;
  app.autoBlockReason="";
}

void setCompressor(bool on) { app.compressorOn=on; actuatorsApplyCompressor(on); }
void setVentEnabled(bool on) {
  app.ventRelayOn=on;
  actuatorsApplyVent(on);
  applyActualFan(activeFanCommandPercent(app.flowControlMode==FLOW_AUTO,app.requestedFanPowerPercent,app.piOutputPercent));
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
}
void stopAutoForSafety(const char *reason) {
  app.flowControlMode=FLOW_MANUAL;
  adoptCurrentFanAsManual();
  app.autoBlocked=true;
  app.autoBlockReason=reason;
}
bool setFlowControlMode(FlowControlMode mode) {
  if(mode==FLOW_AUTO && !flowMeterConversionConfigured()) {
    app.autoBlocked=true; app.autoBlockReason="FLOW CONVERSION MISSING"; return false;
  }
  if(mode==FLOW_AUTO && !app.ventRelayOn) {
    app.autoBlocked=true; app.autoBlockReason="FAN OFF"; return false;
  }
  if(mode==FLOW_AUTO && !autoFeedbackAvailable()) {
    app.autoBlocked=true; app.autoBlockReason="BOTH FLOW SENSORS REQUIRED"; return false;
  }
  if(mode==FLOW_AUTO) piControllerPrepareAuto(app.appliedFanPowerPercent);
  else if(app.flowControlMode==FLOW_AUTO) adoptCurrentFanAsManual();
  app.flowControlMode=mode;
  app.autoBlocked=false;
  app.autoBlockReason="";
  applyActualFan(activeFanCommandPercent(mode==FLOW_AUTO,app.requestedFanPowerPercent,app.piOutputPercent));
  return true;
}
bool setFlowSetpointLpm(float lpm) {
  if(lpm<FLOW_SETPOINT_MIN_LPM || lpm>FLOW_SETPOINT_MAX_LPM) return false;
  app.flowSetpointLpm=lpm;
  pendingApplyInitialize(app.flowSetpointEdit,lpm);
  return true;
}

void editVentPower(int8_t direction,uint32_t now) {
  pendingApplyAdjust(app.ventPowerEdit,direction,1.0f,0.0f,100.0f,now);
}
void editGate(int8_t direction,uint32_t now) {
  pendingApplyAdjust(app.gateEdit,direction,GATE_COMMAND_EDIT_STEP_PERCENT,GATE_COMMAND_MIN_PERCENT,GATE_COMMAND_MAX_PERCENT,now);
}
void editFlowSetpoint(int8_t direction,uint32_t now) {
  pendingApplyAdjust(app.flowSetpointEdit,direction,FLOW_SETPOINT_EDIT_STEP_LPM,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,now);
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
  return true;
}
bool applyFlowSetpointEdit() {
  if(!pendingApplyCommit(app.flowSetpointEdit)) return false;
  app.flowSetpointLpm=app.flowSetpointEdit.applied;
  return true;
}
void appStateService(uint32_t now) {
  pendingApplyTimedOut(app.ventPowerEdit,now,EDIT_APPLY_TIMEOUT_MS);
  pendingApplyTimedOut(app.gateEdit,now,EDIT_APPLY_TIMEOUT_MS);
  pendingApplyTimedOut(app.flowSetpointEdit,now,EDIT_APPLY_TIMEOUT_MS);
}
