#include "pi_controller.h"
#include "app_state.h"
#include "flow_meter.h"
#include "config.h"
#include "pure_logic.h"

static PiControllerCore pi;
static uint32_t lastUpdate=0;

static bool feedback(float &instant,float &filtered) {
  const FlowFeedbackSource source=getFlowFeedbackSource();
  const FlowState &a=app.flow[0], &b=app.flow[1];
  if (source==FLOW_SENSOR_1) {
    if (!a.valid || isnan(a.filteredFlowLpm)) return false;
    instant=a.instantFlowLpm; filtered=a.filteredFlowLpm; return true;
  }
  if (source==FLOW_SENSOR_2) {
    if (!b.valid || isnan(b.filteredFlowLpm)) return false;
    instant=b.instantFlowLpm; filtered=b.filteredFlowLpm; return true;
  }
  const bool aValid=a.valid&&!isnan(a.filteredFlowLpm);
  const bool bValid=b.valid&&!isnan(b.filteredFlowLpm);
  if (!averageOfValid(aValid,a.instantFlowLpm,bValid,b.instantFlowLpm,instant)) return false;
  return averageOfValid(aValid,a.filteredFlowLpm,bValid,b.filteredFlowLpm,filtered);
}

void piControllerBegin() { pi.kp=PI_DEFAULT_KP; pi.tiSeconds=PI_DEFAULT_TI_S; pi.reset(0); }

void piControllerService(uint32_t now) {
  if ((uint32_t)(now-lastUpdate)<PI_UPDATE_INTERVAL_MS) return;
  const float dt=lastUpdate ? (float)(now-lastUpdate)/1000.0f : 1.0f;
  lastUpdate=now;
  float instant, filtered;
  const bool fresh=feedback(instant,filtered);
  if (app.flowControlMode!=FLOW_AUTO) return;
  if (!app.ventRelayOn) { app.autoBlocked=true; app.autoBlockReason="FAN OFF"; return; }
  if (!flowMeterConversionConfigured()) { stopAutoForSafety("FLOW CONVERSION MISSING"); return; }
  if (!fresh) { stopAutoForSafety(flowMeterAutoBlockReason()); return; }

  app.autoBlocked=false;
  app.autoBlockReason="";
  const float normalization=app.flowSetpointLpm>PI_MIN_NORMALIZATION_LPM ? app.flowSetpointLpm : PI_MIN_NORMALIZATION_LPM;
  const float error=(app.flowSetpointLpm-filtered)/normalization;
  const PiTerms terms=pi.update(error,dt,PI_MAX_OUTPUT_STEP_PER_S*dt,PI_INTEGRAL_LIMIT);
  app.piError=error;
  app.piP=terms.p;
  app.piI=terms.i;
  app.piOutputPercent=terms.requested;
  setFanPowerPercent(terms.requested);
}

void piControllerSetKp(float value) { if(value>=0&&value<=20) pi.kp=value; }
void piControllerSetTi(float seconds) { if(seconds>=1&&seconds<=3600) pi.tiSeconds=seconds; }
float piControllerKp() { return pi.kp; }
float piControllerTi() { return pi.tiSeconds; }
void piControllerPrepareAuto(float currentOutput) {
  float instant,filtered;
  const float error=feedback(instant,filtered) ?
    (app.flowSetpointLpm-filtered)/(app.flowSetpointLpm>PI_MIN_NORMALIZATION_LPM ? app.flowSetpointLpm : PI_MIN_NORMALIZATION_LPM) : 0.0f;
  pi.makeBumpless(error,currentOutput);
  app.piOutputPercent=currentOutput;
}
void piControllerPrintStatus() {
  float instant,filtered;
  const bool ok=feedback(instant,filtered);
  const FlowFeedbackSource source=getFlowFeedbackSource();
  Serial.print(F("control mode=")); Serial.print(app.flowControlMode==FLOW_AUTO?F("AUTO"):F("MANUAL"));
  Serial.print(F(" setpoint=")); Serial.print(app.flowSetpointLpm,2); Serial.print(F(" L/min source="));
  Serial.print(source==FLOW_SENSOR_1?F("1"):source==FLOW_SENSOR_2?F("2"):F("avg"));
  Serial.print(F(" instant=")); if(ok)Serial.print(instant,2);else Serial.print(F("ERR"));
  Serial.print(F(" filtered5s=")); if(ok)Serial.print(filtered,2);else Serial.print(F("ERR"));
  Serial.print(F(" L/min error_norm=")); Serial.print(app.piError,4);
  Serial.print(F(" P=")); Serial.print(app.piP,3);
  Serial.print(F(" I=")); Serial.print(app.piI,3);
  Serial.print(F(" requested=")); Serial.print(app.requestedFanPowerPercent,1);
  Serial.print(F(" applied=")); Serial.print(app.appliedFanPowerPercent,1);
  if(app.autoBlocked) { Serial.print(F(" blocked=")); Serial.print(app.autoBlockReason); }
  Serial.println();
}
