#include <string.h>
#include "config.h"
#include "app_state.h"
#include "timer_service.h"
#include "nextion_ui.h"
#include "pure_logic.h"

enum UiFieldIndex : uint8_t {
  UI_TIN, UI_FLOW1, UI_FLOW2, UI_SKIN1, UI_DISTANCE1, UI_SKIN2, UI_DISTANCE2,
  UI_COMPRESSOR, UI_VENT, UI_VENT_SET, UI_GATE_SET, UI_FLOW_SET, UI_TIMER,
  UI_MODE, UI_MODE_STATUS, UI_APPLIED_SETPOINT, UI_AUTO_OUTPUT, UI_ERROR, UI_FIELD_COUNT
};
struct UiField { const char *name; char sent[32]; };
static UiField fields[UI_FIELD_COUNT]={
  {"fTIn",{}},{"fVolume1",{}},{"fVolume2",{}},{"fTSkin1",{}},{"fLSkin1",{}},{"fTSkin2",{}},{"fLSkin2",{}},
  {"fCompStatus",{}},{"fVentStatus",{}},{"tVentSet",{}},{"tGateSet",{}},{"tFlowSet",{}},{"fTimer",{}},
  {"fMode",{}},{"fModeStatus",{}},{"fSetpoint",{}},{"fPiOutput",{}},{"fError",{}}
};
static uint8_t cursor=0, packet[4], packetUsed=0, packetExpected=0;
static const bool NEXTION_DEBUG=true;
static bool lastVentEditing=false, lastGateEditing=false, lastFlowEditing=false;
static bool lastAutoMode=false;

static void endCommand() { NEXTION_SERIAL.write(0xFF); NEXTION_SERIAL.write(0xFF); NEXTION_SERIAL.write(0xFF); }
static void sendText(UiField &field,const char *value) {
  if (!strcmp(field.sent,value)) return;
  NEXTION_SERIAL.print(field.name);
  NEXTION_SERIAL.print(F(".txt=\""));
  NEXTION_SERIAL.print(value);
  NEXTION_SERIAL.print('"');
  endCommand();
  strncpy(field.sent,value,sizeof(field.sent)-1);
  field.sent[sizeof(field.sent)-1]=0;
}
static void formatWholePercent(float value,char *out,size_t size) {
  snprintf(out,size,"%u%%",(unsigned)(clampValue(value,0.0f,100.0f)+0.5f));
}
static void formatSignedPercent(float value,char *out,size_t size) {
  const int command=(int)clampValue(value,GATE_COMMAND_MIN_PERCENT,GATE_COMMAND_MAX_PERCENT);
  if(command>0) snprintf(out,size,"+%d%%",command);
  else snprintf(out,size,"%d%%",command);
}
static void formatWholeLpm(float value,char *out,size_t size) {
  snprintf(out,size,"%u л/мин",(unsigned)(clampValue(value,FLOW_SETPOINT_MIN_LPM,FLOW_SETPOINT_MAX_LPM)+0.5f));
}
static void formatFlowSetpoint(bool defined,float value,char *out,size_t size) {
  if(!defined) { strncpy(out,"NONE",size-1); out[size-1]=0; return; }
  formatWholeLpm(value,out,size);
}
static void fieldValue(UiFieldIndex index,char *out,size_t size,uint32_t now) {
  char value[18];
  out[0]=0;
  switch(index) {
    case UI_TIN:
      if(temperatureStateUsable(app.outletTemperature,now,TEMP_STALE_TIMEOUT_MS)) { formatFixed1(app.outletTemperature.value,value,sizeof(value)); snprintf(out,size,"%s °C",value); }
      else strcpy(out,"—");
      break;
    case UI_FLOW1: case UI_FLOW2: {
      const float flow=index==UI_FLOW1 ? app.modelFlow.output1Lpm : app.modelFlow.output2Lpm;
      formatFixed1(flow,value,sizeof(value)); snprintf(out,size,"%s л/мин",value);
      break;
    }
    case UI_SKIN1: case UI_SKIN2: {
      const TemperatureFaultState &temperature=app.skinTemperature[index==UI_SKIN1?0:1];
      if(temperatureStateUsable(temperature,now,TEMP_STALE_TIMEOUT_MS)) { formatFixed1(temperature.value,value,sizeof(value)); snprintf(out,size,"%s °C",value); }
      else strcpy(out,"—");
      break;
    }
    case UI_DISTANCE1: case UI_DISTANCE2: {
      const TofState &tof=app.tof[index==UI_DISTANCE1?0:1];
      if(tof.valid&&(uint32_t)(now-tof.updatedMs)<=TOF_STALE_MS) snprintf(out,size,"%u mm",tof.distanceMm);
      else strcpy(out,"—");
      break;
    }
    case UI_COMPRESSOR: strcpy(out,app.compressorOn?"ON":"OFF"); break;
    case UI_VENT: strcpy(out,app.ventRelayOn?"ON":"OFF"); break;
    case UI_VENT_SET:
      if(fanSetpointUsesAutoLabel(app.flowControlMode==FLOW_AUTO))strcpy(out,"AUTO");
      else formatWholePercent(app.ventPowerEdit.editing?app.ventPowerEdit.pending:app.ventPowerEdit.applied,out,size);
      break;
    case UI_GATE_SET: formatSignedPercent(app.gateEdit.editing?app.gateEdit.pending:app.gateEdit.applied,out,size); break;
    case UI_FLOW_SET: formatFlowSetpoint(app.flowSetpoint.editing?app.flowSetpoint.pendingDefined:app.flowSetpoint.appliedDefined,app.flowSetpoint.editing?app.flowSetpoint.pendingLpm:app.flowSetpoint.appliedLpm,out,size); break;
    case UI_TIMER: timerServiceFormat(out,size); break;
    case UI_MODE: case UI_MODE_STATUS: strcpy(out,app.flowControlMode==FLOW_AUTO?"AUTO":"MANUAL"); break;
    case UI_APPLIED_SETPOINT:
      if(!app.flowSetpoint.appliedDefined) strcpy(out,"NONE");
      else { formatFixed1(app.flowSetpoint.appliedLpm,value,sizeof(value)); snprintf(out,size,"%s л/мин",value); }
      break;
    case UI_AUTO_OUTPUT:
      if(app.flowControlMode==FLOW_AUTO) { formatFixed1(app.appliedFanPowerPercent,value,sizeof(value)); snprintf(out,size,"%s%%",value); }
      else strcpy(out,"—");
      break;
    case UI_ERROR:
      if(app.flowControlMode==FLOW_AUTO && app.modelStatus!=FLOW_MODEL_OK && app.modelStatus!=FLOW_MODEL_NONE) { strncpy(out,flowModelStatusText(app.modelStatus),size-1); out[size-1]=0; }
      else if(app.outletTemperature.stale) strcpy(out,"TEMP SENSOR STALE");
      else if(app.skinTemperature[0].stale) strcpy(out,"GY906 1 STALE");
      else if(app.skinTemperature[1].stale) strcpy(out,"GY906 2 STALE");
      break;
    default: break;
  }
}
static void updateField(UiFieldIndex index,uint32_t now) {
  char value[32];
  fieldValue(index,value,sizeof(value),now);
  sendText(fields[index],value);
}
static void updateVentSetText(uint32_t now) { updateField(UI_VENT_SET,now); }
static void updateGateSetText(uint32_t now) { updateField(UI_GATE_SET,now); }
static void updateFlowSetText(uint32_t now) { updateField(UI_FLOW_SET,now); }
static void updateModeText(uint32_t now) { updateField(UI_MODE,now); updateField(UI_MODE_STATUS,now); }
static void updateAppliedFlowSetpointText(uint32_t now) { updateField(UI_APPLIED_SETPOINT,now); }
static void syncAll(uint32_t now) { for(uint8_t i=0;i<UI_FIELD_COUNT;++i) updateField((UiFieldIndex)i,now); }

static void handleTrigger(uint8_t id) {
  const uint32_t now=millis();
  if(NEXTION_DEBUG) { Serial.print(F("NEXTION trigger received: 0x")); if(id<16)Serial.print('0'); Serial.println(id,HEX); }
  switch(id) {
    case 0x00: setCompressor(true); updateField(UI_COMPRESSOR,now); break;
    case 0x01: setCompressor(false); updateField(UI_COMPRESSOR,now); break;
    case 0x02: setVentEnabled(true); updateField(UI_VENT,now); break;
    case 0x03: setVentEnabled(false); updateField(UI_VENT,now); break;
    case 0x04: applyVentPowerEdit(); updateVentSetText(now); break;
    case 0x05: applyGateEdit(); updateGateSetText(now); break;
    case 0x06: timerServiceStartOrPause(now); updateField(UI_TIMER,now); break;
    case 0x07: timerServiceReset(now); updateField(UI_TIMER,now); break;
    case 0x08: setFlowControlMode(FLOW_MANUAL); updateModeText(now); updateVentSetText(now); break;
    case 0x09: setFlowControlMode(FLOW_AUTO); updateModeText(now); updateVentSetText(now); updateField(UI_ERROR,now); break;
    case 0x0A: applyFlowSetpointEdit(); updateFlowSetText(now); updateAppliedFlowSetpointText(now); break;
    case 0x0B: editVentPower(-1,now); updateVentSetText(now); break;
    case 0x0C: editVentPower(1,now); updateVentSetText(now); break;
    case 0x0D: editGate(-1,now); updateGateSetText(now); break;
    case 0x0E: editGate(1,now); updateGateSetText(now); break;
    case 0x0F: editFlowSetpoint(-1,now); updateFlowSetText(now); break;
    case 0x10: editFlowSetpoint(1,now); updateFlowSetText(now); break;
    default: if(NEXTION_DEBUG) { Serial.print(F("NEXTION unknown trigger: ")); Serial.println(id); } break;
  }
}
static void processInput() {
  while(NEXTION_SERIAL.available()) {
    const uint8_t byte=NEXTION_SERIAL.read();
    if(!packetUsed) {
      if(byte==0x23) { packet[0]=byte; packetUsed=1; packetExpected=0; }
      continue;
    }
    if(packetUsed<sizeof(packet)) packet[packetUsed++]=byte;
    if(packetUsed==2) packetExpected=(uint8_t)(2+packet[1]);
    if(packetExpected&&packetUsed>=packetExpected) {
      if(packetExpected==4&&packet[1]>=2&&packet[2]=='T') handleTrigger(packet[3]);
      else if(NEXTION_DEBUG) Serial.println(F("NEXTION malformed trigger ignored"));
      packetUsed=packetExpected=0;
    }
  }
}

void nextionUiBegin() {
  NEXTION_SERIAL.begin(NEXTION_BAUD);
  if(NEXTION_DEBUG) { Serial.print(F("NEXTION debug enabled, baud=")); Serial.println(NEXTION_BAUD); }
  syncAll(millis());
  lastVentEditing=app.ventPowerEdit.editing;
  lastGateEditing=app.gateEdit.editing;
  lastFlowEditing=app.flowSetpoint.editing;
  lastAutoMode=app.flowControlMode==FLOW_AUTO;
}
void nextionUiService(uint32_t now) {
  processInput();
  const bool autoMode=app.flowControlMode==FLOW_AUTO;
  if(lastAutoMode!=autoMode) {
    updateModeText(now);
    updateVentSetText(now);
    updateField(UI_ERROR,now);
    lastAutoMode=autoMode;
  }
  if(lastVentEditing!=app.ventPowerEdit.editing) { updateVentSetText(now); lastVentEditing=app.ventPowerEdit.editing; }
  if(lastGateEditing!=app.gateEdit.editing) { updateGateSetText(now); lastGateEditing=app.gateEdit.editing; }
  if(lastFlowEditing!=app.flowSetpoint.editing) { updateFlowSetText(now); lastFlowEditing=app.flowSetpoint.editing; }
  for(uint8_t count=0;count<3;++count) updateField((UiFieldIndex)(cursor++%UI_FIELD_COUNT),now);
}
