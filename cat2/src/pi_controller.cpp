#include "pi_controller.h"
#include "app_state.h"
#include "flow_meter.h"
#include "config.h"
#include "pure_logic.h"

static PiControllerCore pi;
static uint32_t lastUpdate=0;

static bool totalFeedback(float &flow1,float &flow2,float &total) {
  const FlowState &a=app.flow[0], &b=app.flow[1];
  const bool aValid=a.valid&&!isnan(a.filteredFlowLpm);
  const bool bValid=b.valid&&!isnan(b.filteredFlowLpm);
  if(!totalFlowFromBothValid(aValid,a.filteredFlowLpm,bValid,b.filteredFlowLpm,total)) return false;
  flow1=a.filteredFlowLpm;
  flow2=b.filteredFlowLpm;
  return true;
}
static float piErrorPercent(float totalLpm) {
  return normalizedFlowErrorPercent(app.flowSetpointLpm,totalLpm,PI_MIN_NORMALIZATION_LPM);
}

void piControllerBegin() { pi.kp=PI_DEFAULT_KP; pi.tiSeconds=PI_DEFAULT_TI_S; pi.reset(0); }

void piControllerService(uint32_t now) {
  if((uint32_t)(now-lastUpdate)<PI_UPDATE_INTERVAL_MS) return;
  const float dt=lastUpdate ? (float)(now-lastUpdate)/1000.0f : 1.0f;
  lastUpdate=now;
  float flow1,flow2,total;
  const bool fresh=totalFeedback(flow1,flow2,total);
  if(app.flowControlMode!=FLOW_AUTO) return;
  if(!app.ventRelayOn) { app.autoBlocked=true; app.autoBlockReason="FAN OFF"; return; }
  if(!flowMeterConversionConfigured()) { stopAutoForSafety("FLOW CONVERSION MISSING"); return; }
  if(!fresh) { stopAutoForSafety(flowMeterAutoBlockReason()); return; }

  app.autoBlocked=false;
  app.autoBlockReason="";
  app.piFlowErrorLpm=app.flowSetpointLpm-total;
  const float error=piErrorPercent(total);
  const PiTerms terms=pi.update(error,dt,PI_OUTPUT_RISE_RATE_PERCENT_PER_SEC,
                                 PI_OUTPUT_FALL_RATE_PERCENT_PER_SEC,PI_INTEGRAL_LIMIT);
  app.piError=error;
  app.piP=terms.p;
  app.piI=terms.i;
  app.piRawOutputPercent=terms.rawOutput;
  app.piClampedOutputPercent=terms.clampedOutput;
  app.piOutputPercent=terms.requested;
  setAutomaticFanPowerPercent(terms.requested);
}

void piControllerSetKp(float value) { if(value>=0&&value<=20) pi.kp=value; }
void piControllerSetTi(float seconds) { if(seconds>=1&&seconds<=3600) pi.tiSeconds=seconds; }
float piControllerKp() { return pi.kp; }
float piControllerTi() { return pi.tiSeconds; }
void piControllerPrepareAuto(float currentOutput) {
  float flow1,flow2,total;
  const float error=totalFeedback(flow1,flow2,total) ? piErrorPercent(total) : 0.0f;
  pi.makeBumpless(error,currentOutput);
  app.piOutputPercent=currentOutput;
  app.piRawOutputPercent=currentOutput;
  app.piClampedOutputPercent=currentOutput;
}
void piControllerPrintStatus() {
  float flow1,flow2,total;
  const bool valid=totalFeedback(flow1,flow2,total);
  const bool flow1Valid=app.flow[0].valid&&!isnan(app.flow[0].filteredFlowLpm);
  const bool flow2Valid=app.flow[1].valid&&!isnan(app.flow[1].filteredFlowLpm);
  const float requested=activeFanCommandPercent(app.flowControlMode==FLOW_AUTO,
                                                 app.requestedFanPowerPercent,
                                                 app.piOutputPercent);
  Serial.print(F("Mode: ")); Serial.println(app.flowControlMode==FLOW_AUTO?F("AUTO"):F("MANUAL"));
  Serial.print(F("Flow1 filtered: ")); if(flow1Valid)Serial.print(app.flow[0].filteredFlowLpm,2);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
  Serial.print(F("Flow2 filtered: ")); if(flow2Valid)Serial.print(app.flow[1].filteredFlowLpm,2);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
  Serial.print(F("Total flow: ")); if(valid)Serial.print(total,2);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
  Serial.print(F("Flow setpoint: ")); Serial.print(app.flowSetpointLpm,2); Serial.println(F(" L/min"));
  Serial.print(F("Flow error: ")); if(valid)Serial.print(app.flowSetpointLpm-total,2);else Serial.print(F("ERR")); Serial.println(F(" L/min"));
  Serial.print(F("PI normalized error: ")); Serial.println(app.piError,3);
  Serial.print(F("PI P: ")); Serial.print(app.piP,2); Serial.print(F(" I: ")); Serial.println(app.piI,2);
  Serial.print(F("PI raw output: ")); Serial.print(app.piRawOutputPercent,2); Serial.println(F("%"));
  Serial.print(F("PI clamped output: ")); Serial.print(app.piClampedOutputPercent,2); Serial.println(F("%"));
  Serial.print(F("PI rate-limited output: ")); Serial.print(app.piOutputPercent,2); Serial.println(F("%"));
  Serial.print(F("Requested fan output: ")); Serial.print(requested,2); Serial.println(F("%"));
  Serial.print(F("Fan manual applied/pending: ")); Serial.print(app.ventPowerEdit.applied,1); Serial.print('/'); Serial.print(app.ventPowerEdit.pending,1); Serial.println(F("%"));
  Serial.print(F("Fan actual commanded: ")); Serial.print(app.appliedFanPowerPercent,2); Serial.println(F("%"));
  Serial.print(F("Fan PWM raw: ")); Serial.println(app.fanPwmRaw);
  Serial.print(F("Vent relay: ")); Serial.println(app.ventRelayOn?F("ON"):F("OFF"));
  Serial.print(F("PWM inverted: ")); Serial.println(FAN_PWM_INVERTED?F("true"):F("false"));
  const TemperatureFaultState &temperature=app.outletTemperature;
  const uint32_t now=millis();
  Serial.print(F("Density temperature: "));
  if(temperature.hasLastGood)Serial.print(temperature.lastGoodValue,2);else Serial.print(F("ERR"));
  Serial.print(F(" C valid="));Serial.print(temperatureStateUsable(temperature,now,TEMP_STALE_TIMEOUT_MS)?F("yes"):F("no"));
  Serial.print(F(" stale="));Serial.print(temperature.stale?F("yes"):F("no"));
  Serial.print(F(" age_ms="));
  if(temperature.hasLastGood)Serial.println(temperatureStateAgeMs(temperature,now));else Serial.println(F("never"));
  Serial.print(F("Block reason: "));
  if (app.autoBlocked) {
    Serial.println(app.autoBlockReason);
  } else {
    Serial.println(F("NONE"));
  }
}
