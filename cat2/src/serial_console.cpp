#include <string.h>
#include <stdlib.h>
#include "serial_console.h"
#include "config.h"
#include "app_state.h"
#include "flow_meter.h"
#include "i2c_mux.h"
#include "temperature_sensors.h"
#include "tof_sensors.h"
#include "timer_service.h"
#include "pi_controller.h"

static char line[96];
static uint8_t used=0, debugLevel=0;
static void help() {
  Serial.println(F("commands: status, comp on|off, vent on|off|<%>, gate <%, flow [raw|density|reset],"));
  Serial.println(F("timer start|pause|reset|status, i2c scan|mux, tof on|off|scan|status|read,"));
  Serial.println(F("control manual|auto|setpoint <L/min>|kp <v>|ti <s>|status, debug off|sensors|all"));
}
static bool number(const char *s,float &v) { if(!s||!*s)return false; char *end; v=(float)strtod(s,&end); return *end==0; }
static void printEditState(const __FlashStringHelper *name,const PendingApplyState &state,const __FlashStringHelper *unit) {
  const uint32_t now=millis();
  Serial.print(name); Serial.print(F(": applied=")); Serial.print(state.applied,1); Serial.print(unit);
  Serial.print(F(" pending=")); Serial.print(state.pending,1); Serial.print(unit);
  Serial.print(F(" editing=")); Serial.print(state.editing?F("yes"):F("no"));
  if(state.editing) { Serial.print(F(" age_ms=")); Serial.print(pendingApplyAgeMs(state,now)); Serial.print(F(" remaining_ms=")); Serial.print(pendingApplyRemainingMs(state,now,EDIT_APPLY_TIMEOUT_MS)); }
  Serial.println();
}

void serialConsolePrintStatus() {
  Serial.print(F("actuators: compressor=")); Serial.print(app.compressorOn?F("ON"):F("OFF"));
  Serial.print(F(" vent=")); Serial.print(app.ventRelayOn?F("ON"):F("OFF"));
  Serial.print(F(" fan requested/applied=")); Serial.print(app.requestedFanPowerPercent,1); Serial.print('/'); Serial.print(app.appliedFanPowerPercent,1);
  Serial.print(F(" gate_command=")); Serial.print(app.gateCommandPercent,1); Serial.print(F("% physical=")); Serial.print(app.gatePercent,1); Serial.print(F("% angle=")); Serial.println(app.gateAngle);
  printEditState(F("Vent manual"),app.ventPowerEdit,F("%"));
  printEditState(F("Gate"),app.gateEdit,F("%"));
  printEditState(F("Flow setpoint"),app.flowSetpointEdit,F(" L/min"));
  temperatureSensorsPrintStatus(); flowMeterPrintStatus(false); tofSensorsPrintStatus();
  char timer[16]; timerServiceFormat(timer,sizeof(timer));
  Serial.print(F("timer=")); Serial.print(timer); Serial.print(F(" flow_conversion="));
  Serial.println(flowMeterConversionConfigured()?F("configured"):F("missing"));
  piControllerPrintStatus();
}

static void handle(char *s) {
  char *cmd=strtok(s," \t"), *arg=strtok(0," \t"), *arg2=strtok(0," \t");
  if(!cmd) return;
  if(!strcmp(cmd,"help")||!strcmp(cmd,"?")) { help(); return; }
  if(!strcmp(cmd,"status")) { serialConsolePrintStatus(); return; }
  if(!strcmp(cmd,"comp")) { if(arg&&!strcmp(arg,"on"))setCompressor(true); else if(arg&&!strcmp(arg,"off"))setCompressor(false); else help(); return; }
  if(!strcmp(cmd,"vent")) { float v; if(arg&&!strcmp(arg,"on"))setVentEnabled(true); else if(arg&&!strcmp(arg,"off"))setVentEnabled(false); else if(number(arg,v)&&v>=0&&v<=100)setFanPowerPercent(v); else help(); return; }
  if(!strcmp(cmd,"gate")||!strcmp(cmd,"servo")) { float v; if(number(arg,v)&&v>=GATE_COMMAND_MIN_PERCENT&&v<=GATE_COMMAND_MAX_PERCENT)setGateCommandPercent((int16_t)v); else help(); return; }
  if(!strcmp(cmd,"flow")) { if(arg&&!strcmp(arg,"reset"))flowMeterReset(); else if(arg&&!strcmp(arg,"density"))flowMeterPrintDensity(); else flowMeterPrintStatus(arg&&!strcmp(arg,"raw")); return; }
  if(!strcmp(cmd,"timer")) { if(arg&&!strcmp(arg,"start"))timerServiceStart(millis()); else if(arg&&!strcmp(arg,"pause"))timerServicePause(millis()); else if(arg&&!strcmp(arg,"reset"))timerServiceReset(millis()); else if(arg&&!strcmp(arg,"status")){char b[16];timerServiceFormat(b,sizeof(b));Serial.println(b);} else help(); return; }
  if(!strcmp(cmd,"i2c")) { if(arg&&!strcmp(arg,"scan"))i2cMuxScanBus(); else if(arg&&!strcmp(arg,"mux"))i2cMuxScanChannels(); else help(); return; }
  if(!strcmp(cmd,"tof")) { if(arg&&!strcmp(arg,"on"))tofSensorsSetEnabled(true); else if(arg&&!strcmp(arg,"off"))tofSensorsSetEnabled(false); else if(arg&&!strcmp(arg,"scan"))tofSensorsScan(); else if(arg&&!strcmp(arg,"status"))tofSensorsPrintStatus(); else if(arg&&!strcmp(arg,"read"))tofSensorsReadNow(); else help(); return; }
  if(!strcmp(cmd,"control")) {
    float v;
    if(arg&&!strcmp(arg,"manual")) setFlowControlMode(FLOW_MANUAL);
    else if(arg&&!strcmp(arg,"auto")) { if(!setFlowControlMode(FLOW_AUTO)){Serial.print(F("AUTO blocked: "));Serial.println(app.autoBlockReason);} }
    else if(arg&&!strcmp(arg,"setpoint")&&number(arg2,v)) { if(!setFlowSetpointLpm(v))Serial.println(F("ERR: setpoint must be 0..100 L/min")); }
    else if(arg&&!strcmp(arg,"kp")&&number(arg2,v)) piControllerSetKp(v);
    else if(arg&&!strcmp(arg,"ti")&&number(arg2,v)) piControllerSetTi(v);
    else if(arg&&!strcmp(arg,"status")) piControllerPrintStatus(); else help();
    return;
  }
  if(!strcmp(cmd,"debug")) { if(arg&&!strcmp(arg,"off"))debugLevel=0; else if(arg&&!strcmp(arg,"sensors"))debugLevel=1; else if(arg&&!strcmp(arg,"all"))debugLevel=2; else help(); return; }
  help();
}

void serialConsoleBegin() { Serial.begin(9600); help(); }
void serialConsoleService() {
  while(Serial.available()) {
    char c=(char)Serial.read();
    if(c=='\r'||c=='\n') { if(used){line[used]=0;handle(line);used=0;} }
    else if(used<sizeof(line)-1) line[used++]=c;
    else { used=0;Serial.println(F("ERR: command too long")); }
  }
  static uint32_t last=0;
  if(debugLevel&&(uint32_t)(millis()-last)>=1000) { last=millis(); if(debugLevel>=1)flowMeterPrintStatus(false); if(debugLevel>=2){temperatureSensorsPrintStatus();tofSensorsPrintStatus();} }
}
