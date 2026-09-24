#include "model_control.h"
#include "app_state.h"
#include "actuators.h"
#include "flow_model.h"

namespace {
bool autoUpdateRequested=true;

static void refreshActualEstimate() {
  const float actualPower=app.ventRelayOn ? app.appliedFanPowerPercent : 0.0f;
  app.modelFlow=estimateFlow(actualPower,(int16_t)app.gateCommandPercent);
  if (app.flowControlMode!=FLOW_AUTO) {
    app.modelStatus=flowModelStatusForPower(actualPower,app.ventRelayOn);
    app.modelTargetFanPowerPercent=actualPower;
  }
}
}

void modelControlBegin() { autoUpdateRequested=true; refreshActualEstimate(); }
void modelControlRequestAutoUpdate() { autoUpdateRequested=true; }

void modelControlService(uint32_t) {
  if (app.flowControlMode==FLOW_AUTO && autoUpdateRequested) {
    if (!app.flowSetpoint.appliedDefined) {
      app.modelTargetFanPowerPercent=0.0f;
      app.modelStatus=FLOW_MODEL_NONE;
    } else {
      const FlowModelSolveResult result=solveFlowModel(app.flowSetpoint.appliedLpm,(int16_t)app.gateCommandPercent);
      app.modelTargetFanPowerPercent=result.fanPowerPercent;
      app.modelStatus=result.status;
    }
    // This only changes PWM through the actuator authority. It never toggles
    // the relay; with the relay OFF the applied power remains zero.
    setAutomaticFanPowerPercent(app.modelTargetFanPowerPercent);
    autoUpdateRequested=false;
  }
  refreshActualEstimate();
}

void modelControlPrintStatus() {
  Serial.print(F("Mode: ")); Serial.println(app.flowControlMode==FLOW_AUTO?F("AUTO"):F("MANUAL"));
  Serial.print(F("Applied flow target: "));
  if(app.flowSetpoint.appliedDefined) { Serial.print(app.flowSetpoint.appliedLpm,1); Serial.println(F(" L/min")); }
  else Serial.println(F("NONE"));
  Serial.print(F("Gate applied: ")); Serial.print(app.gateCommandPercent,0); Serial.println(F("%"));
  Serial.print(F("Fan actual: ")); Serial.print(app.appliedFanPowerPercent,1); Serial.println(F("%"));
  Serial.print(F("Fan model-derived target: ")); Serial.print(app.modelTargetFanPowerPercent,1); Serial.println(F("%"));
  Serial.print(F("Vent relay: ")); Serial.print(app.ventRelayOn?F("ON"):F("OFF"));
  Serial.print(F(" raw PWM: ")); Serial.println(app.fanPwmRaw);
  Serial.print(F("Calculated Q1/Q2/total: ")); Serial.print(app.modelFlow.output1Lpm,1); Serial.print('/');
  Serial.print(app.modelFlow.output2Lpm,1); Serial.print('/'); Serial.print(app.modelFlow.totalLpm,1); Serial.println(F(" L/min"));
  Serial.print(F("Model status: ")); Serial.println(flowModelStatusText(app.modelStatus));
}
