#include "app_state.h"
#include "config.h"
#include "actuators.h"
#include "flow_meter.h"
#include "pi_controller.h"

AppState app;
static FlowFeedbackSource feedbackSource=(FlowFeedbackSource)PI_DEFAULT_FEEDBACK_SOURCE;
static float clampPercent(float x) { return x<0 ? 0 : (x>100 ? 100 : x); }
static bool autoFeedbackAvailable() {
  const FlowState &a=app.flow[0], &b=app.flow[1];
  if (feedbackSource==FLOW_SENSOR_1) return a.valid&&a.conversionConfigured;
  if (feedbackSource==FLOW_SENSOR_2) return b.valid&&b.conversionConfigured;
  return (a.valid&&a.conversionConfigured)||(b.valid&&b.conversionConfigured);
}

void appStateBegin() {
  app.compressorOn=false; app.ventRelayOn=false;
  app.requestedFanPowerPercent=0; app.appliedFanPowerPercent=0;
  app.gatePercent=0; app.gateAngle=GATE_ANGLE_MIN;
  app.flowControlMode=FLOW_MANUAL; app.flowSetpointLpm=0;
  app.timerRunning=false; app.timerElapsedMs=0;
  app.outletTemperature={0,false,0};
  for(uint8_t i=0;i<2;++i) {
    app.skinTemperature[i]={0,false,0};
    app.flow[i]={0,0,0,0,NAN,NAN,NAN,NAN,NAN,false,true,false,FLOW_CONVERSION_CONFIGURED,false};
    app.tof[i]={255,0,0,0,false,false,false,false,false,0};
  }
  app.piOutputPercent=0; app.piError=app.piP=app.piI=0;
  app.autoBlocked=false; app.autoBlockReason="";
}

void setCompressor(bool on) { app.compressorOn=on; actuatorsApplyCompressor(on); }
void setVentEnabled(bool on) {
  app.ventRelayOn=on;
  app.appliedFanPowerPercent=on ? app.requestedFanPowerPercent : 0;
  actuatorsApplyVent(on);
  actuatorsApplyFan(app.appliedFanPowerPercent);
}
void setFanPowerPercent(float percent) {
  app.requestedFanPowerPercent=clampPercent(percent);
  app.appliedFanPowerPercent=app.ventRelayOn ? app.requestedFanPowerPercent : 0;
  actuatorsApplyFan(app.appliedFanPowerPercent);
}
void setGatePercent(float percent) {
  app.gatePercent=clampPercent(percent);
  app.gateAngle=(uint8_t)(GATE_ANGLE_MIN+(app.gatePercent*(GATE_ANGLE_MAX-GATE_ANGLE_MIN)/100.0f)+0.5f);
  actuatorsApplyGate(app.gateAngle);
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
  if (mode==FLOW_AUTO) piControllerPrepareAuto(app.requestedFanPowerPercent);
  app.flowControlMode=mode;
  app.autoBlocked=false;
  app.autoBlockReason="";
  return true;
}
bool setFlowSetpointLpm(float lpm) {
  if (lpm<FLOW_SETPOINT_MIN_LPM || lpm>FLOW_SETPOINT_MAX_LPM) return false;
  app.flowSetpointLpm=lpm;
  return true;
}
void setFlowFeedbackSource(FlowFeedbackSource source) { feedbackSource=source; }
FlowFeedbackSource getFlowFeedbackSource() { return feedbackSource; }
