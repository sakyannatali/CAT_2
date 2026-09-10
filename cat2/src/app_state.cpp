#include "app_state.h"
#include "config.h"
#include "actuators.h"
#include "flow_meter.h"
#include "pi_controller.h"

AppState app;
static FlowFeedbackSource feedbackSource=(FlowFeedbackSource)PI_DEFAULT_FEEDBACK_SOURCE;

static bool autoFeedbackAvailable() {
  const FlowState &a=app.flow[0], &b=app.flow[1];
  if (feedbackSource==FLOW_SENSOR_1) return a.valid&&a.conversionConfigured;
  if (feedbackSource==FLOW_SENSOR_2) return b.valid&&b.conversionConfigured;
  return (a.valid&&a.conversionConfigured)||(b.valid&&b.conversionConfigured);
}
static uint8_t gateAngleFor(float percent) {
  return (uint8_t)(GATE_ANGLE_MIN+(percent*(GATE_ANGLE_MAX-GATE_ANGLE_MIN)/100.0f)+0.5f);
}
static void applyActualFan(float percent) {
  app.appliedFanPowerPercent=app.ventRelayOn ? clampValue(percent,0.0f,100.0f) : 0.0f;
  actuatorsApplyFan(app.appliedFanPowerPercent);
}
static void applyGatePosition(float percent) {
  app.gatePercent=clampValue(percent,0.0f,100.0f);
  app.gateAngle=gateAngleFor(app.gatePercent);
  actuatorsApplyGate(app.gateAngle);
}

void appStateBegin() {
  app.compressorOn=false;
  app.ventRelayOn=false;
  app.requestedFanPowerPercent=0;
  app.appliedFanPowerPercent=0;
  app.gatePercent=0;
  app.gateAngle=GATE_ANGLE_MIN;
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
  app.piError=app.piP=app.piI=0;
  app.autoBlocked=false;
  app.autoBlockReason="";
}

void setCompressor(bool on) { app.compressorOn=on; actuatorsApplyCompressor(on); }
void setVentEnabled(bool on) {
  app.ventRelayOn=on;
  actuatorsApplyVent(on);
  applyActualFan(app.flowControlMode==FLOW_AUTO ? app.piOutputPercent : app.requestedFanPowerPercent);
}
void setFanPowerPercent(float percent) {
  app.requestedFanPowerPercent=clampValue(percent,0.0f,100.0f);
  pendingApplyInitialize(app.ventPowerEdit,app.requestedFanPowerPercent);
  // In AUTO this is stored as the next MANUAL value; PI alone controls PWM.
  if (app.flowControlMode==FLOW_MANUAL) applyActualFan(app.requestedFanPowerPercent);
}
void setAutomaticFanPowerPercent(float percent) {
  applyActualFan(percent);
}
void setGatePercent(float percent) {
  const float applied=clampValue(percent,0.0f,100.0f);
  pendingApplyInitialize(app.gateEdit,applied);
  applyGatePosition(applied);
}
void stopAutoForSafety(const char *reason) {
  app.flowControlMode=FLOW_MANUAL;
  app.autoBlocked=true;
  app.autoBlockReason=reason;
}
bool setFlowControlMode(FlowControlMode mode) {
  if (mode==FLOW_AUTO && !flowMeterConversionConfigured()) {
    app.autoBlocked=true; app.autoBlockReason="FLOW CONVERSION MISSING"; return false;
  }
  if (mode==FLOW_AUTO && !app.ventRelayOn) {
    app.autoBlocked=true; app.autoBlockReason="FAN OFF"; return false;
  }
  if (mode==FLOW_AUTO && !autoFeedbackAvailable()) {
    app.autoBlocked=true; app.autoBlockReason=flowMeterAutoBlockReason(); return false;
  }
  if (mode==FLOW_AUTO) piControllerPrepareAuto(app.appliedFanPowerPercent);
  app.flowControlMode=mode;
  app.autoBlocked=false;
  app.autoBlockReason="";
  applyActualFan(mode==FLOW_AUTO ? app.piOutputPercent : app.requestedFanPowerPercent);
  return true;
}
bool setFlowSetpointLpm(float lpm) {
  if (lpm<FLOW_SETPOINT_MIN_LPM || lpm>FLOW_SETPOINT_MAX_LPM) return false;
  app.flowSetpointLpm=lpm;
  pendingApplyInitialize(app.flowSetpointEdit,lpm);
  return true;
}

void editVentPower(int8_t direction, uint32_t now) {
  pendingApplyAdjust(app.ventPowerEdit,direction,1.0f,0.0f,100.0f,now);
}
void editGate(int8_t direction, uint32_t now) {
  pendingApplyAdjust(app.gateEdit,direction,1.0f,0.0f,100.0f,now);
}
void editFlowSetpoint(int8_t direction, uint32_t now) {
  pendingApplyAdjust(app.flowSetpointEdit,direction,1.0f,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM,now);
}
bool applyVentPowerEdit() {
  if (!pendingApplyCommit(app.ventPowerEdit)) return false;
  app.requestedFanPowerPercent=app.ventPowerEdit.applied;
  if (app.flowControlMode==FLOW_MANUAL) applyActualFan(app.requestedFanPowerPercent);
  return true;
}
bool applyGateEdit() {
  if (!pendingApplyCommit(app.gateEdit)) return false;
  applyGatePosition(app.gateEdit.applied);
  return true;
}
bool applyFlowSetpointEdit() {
  if (!pendingApplyCommit(app.flowSetpointEdit)) return false;
  app.flowSetpointLpm=app.flowSetpointEdit.applied;
  return true;
}
void appStateService(uint32_t now) {
  pendingApplyTimedOut(app.ventPowerEdit,now,EDIT_APPLY_TIMEOUT_MS);
  pendingApplyTimedOut(app.gateEdit,now,EDIT_APPLY_TIMEOUT_MS);
  pendingApplyTimedOut(app.flowSetpointEdit,now,EDIT_APPLY_TIMEOUT_MS);
}
void setFlowFeedbackSource(FlowFeedbackSource source) { feedbackSource=source; }
FlowFeedbackSource getFlowFeedbackSource() { return feedbackSource; }
