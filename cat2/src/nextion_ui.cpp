#include <string.h>
#include "config.h"
#include "app_state.h"
#include "flow_meter.h"
#include "timer_service.h"
#include "nextion_ui.h"
#include "pure_logic.h"

struct UiField { const char *name; char sent[32]; };
static UiField fields[]={{"fTIn",{}},{"fVolume1",{}},{"fVolume2",{}},{"fTSkin1",{}},{"fLSkin1",{}},{"fTSkin2",{}},{"fLSkin2",{}},{"fCompStatus",{}},{"fVentStatus",{}},{"tVentSpeed",{}},{"tGate",{}},{"fTimer",{}},{"fMode",{}},{"fSetpoint",{}},{"fPiOutput",{}},{"fError",{}}};
static uint8_t cursor=0;
static uint8_t packet[8], packetUsed=0, packetExpected=0, pendingNumber=0;
static uint32_t pendingSinceMs=0;
static const bool NEXTION_DEBUG = true;
static const char *pendingTarget(uint8_t which){if(which==4)return "fVentPower.val";if(which==5)return "fGate.val";if(which==10)return "nFlowSetpoint.val";return "?";}
static const __FlashStringHelper *nextionStatusName(uint8_t code){switch(code){case 0x00:return F("invalid instruction");case 0x01:return F("success");case 0x02:return F("invalid component ID");case 0x03:return F("invalid page ID");case 0x04:return F("invalid picture ID");case 0x05:return F("invalid font ID");case 0x11:return F("invalid baud");case 0x12:return F("invalid curve/control");case 0x1A:return F("invalid variable");case 0x1B:return F("invalid operation");case 0x1C:return F("assignment failed");case 0x1D:return F("EEPROM failed");case 0x1E:return F("parameter quantity invalid");case 0x1F:return F("IO operation failed");case 0x20:return F("escape character invalid");case 0x23:return F("variable name too long");default:return F("status");}}
static void sendText(UiField &field,const char *value){if(!strcmp(field.sent,value))return;NEXTION_SERIAL.print(field.name);NEXTION_SERIAL.print(F(".txt=\""));NEXTION_SERIAL.print(value);NEXTION_SERIAL.print('"');NEXTION_SERIAL.write(0xFF);NEXTION_SERIAL.write(0xFF);NEXTION_SERIAL.write(0xFF);strncpy(field.sent,value,sizeof(field.sent)-1);field.sent[sizeof(field.sent)-1]=0;}
static void fieldValue(uint8_t index,char*out,size_t size,uint32_t now){char v[18];out[0]=0;switch(index){
  case 0: if(app.outletTemperature.valid&&(uint32_t)(now-app.outletTemperature.updatedMs)<=TEMPERATURE_STALE_MS){formatFixed1(app.outletTemperature.value,v,sizeof(v));snprintf(out,size,"%s °C",v);}else strcpy(out,"ERR");break;
  case 1: case 2:{const FlowState&s=app.flow[index-1];if(!s.valid||isnan(s.displayFlowLpm))strcpy(out,"—");else{formatFixed1(s.displayFlowLpm,v,sizeof(v));snprintf(out,size,"%s л/мин",v);}break;}
  case 3: case 5:{const ValueState&s=app.skinTemperature[(index-3)/2];if(s.valid&&(uint32_t)(now-s.updatedMs)<=TEMPERATURE_STALE_MS){formatFixed1(s.value,v,sizeof(v));snprintf(out,size,"%s °C",v);}else strcpy(out,"ERR");break;}
  case 4: case 6:{const TofState&s=app.tof[(index-4)/2];if(s.valid&&(uint32_t)(now-s.updatedMs)<=TOF_STALE_MS)snprintf(out,size,"%u mm",s.distanceMm);else strcpy(out,"—");break;}
  case 7:strcpy(out,app.compressorOn?"ON":"OFF");break; case 8:strcpy(out,app.ventRelayOn?"ON":"OFF");break;
  case 9:formatFixed1(app.appliedFanPowerPercent,v,sizeof(v));snprintf(out,size,"%s %%",v);break; case 10:formatFixed1(app.gatePercent,v,sizeof(v));snprintf(out,size,"%s %%",v);break;
  case 11:timerServiceFormat(out,size);break;case 12:strcpy(out,app.flowControlMode==FLOW_AUTO?"AUTO":"MANUAL");break;
  case 13:formatFixed1(app.flowSetpointLpm,v,sizeof(v));snprintf(out,size,"%s л/мин",v);break;case 14:formatFixed1(app.piOutputPercent,v,sizeof(v));snprintf(out,size,"%s %%",v);break;
  default: if(app.autoBlocked){strncpy(out,app.autoBlockReason,size-1);out[size-1]=0;}else if(!app.flow[0].densityValid&&!app.flow[1].densityValid){strncpy(out,flowMeterAutoBlockReason(),size-1);out[size-1]=0;}else out[0]=0;break;
}}
static void endCommand(){NEXTION_SERIAL.write(0xFF);NEXTION_SERIAL.write(0xFF);NEXTION_SERIAL.write(0xFF);}
static void requestNumber(uint8_t which){pendingNumber=which;pendingSinceMs=millis();if(NEXTION_DEBUG){Serial.print(F("NEXTION request: get "));Serial.println(pendingTarget(which));}if(which==4)NEXTION_SERIAL.print(F("get fVentPower.val"));else if(which==5)NEXTION_SERIAL.print(F("get fGate.val"));else NEXTION_SERIAL.print(F("get nFlowSetpoint.val"));endCommand();}
static void handleTrigger(uint8_t id){if(NEXTION_DEBUG){Serial.print(F("NEXTION trigger received: "));Serial.println(id);}switch(id){case 0:setCompressor(true);break;case 1:setCompressor(false);break;case 2:setVentEnabled(true);break;case 3:setVentEnabled(false);break;case 4:case 5:case 10:requestNumber(id);break;case 6:timerServiceStartOrPause(millis());break;case 7:timerServiceReset(millis());break;case 8:setFlowControlMode(FLOW_MANUAL);break;case 9:setFlowControlMode(FLOW_AUTO);break;default:if(NEXTION_DEBUG){Serial.print(F("NEXTION unknown trigger: "));Serial.println(id);}break;}}
static void processInput(){while(NEXTION_SERIAL.available()){
  const uint8_t b=NEXTION_SERIAL.read();
  if(!packetUsed){if(b==0x23){packet[0]=b;packetUsed=1;packetExpected=0;}else if(b==0x71){packet[0]=b;packetUsed=1;packetExpected=8;}else if(b<=0x24){packet[0]=b;packetUsed=1;packetExpected=4;}continue;}
  if(packetUsed<sizeof(packet))packet[packetUsed++]=b;
  if(packetUsed==2&&packet[0]==0x23)packetExpected=(uint8_t)(2+packet[1]);
  if(packetExpected&&packetUsed>=packetExpected){
    if(packet[0]==0x23&&packet[1]>=2&&packet[2]=='T')handleTrigger(packet[3]);
    else if(packet[0]==0x71&&packetUsed==8&&packet[5]==0xFF&&packet[6]==0xFF&&packet[7]==0xFF){const uint32_t v=(uint32_t)packet[1]|((uint32_t)packet[2]<<8)|((uint32_t)packet[3]<<16)|((uint32_t)packet[4]<<24);if(NEXTION_DEBUG){Serial.print(F("NEXTION number response: "));Serial.print(pendingTarget(pendingNumber));Serial.print(F(" = "));Serial.println(v);}if(pendingNumber==4)setFanPowerPercent(v>100?100:v);else if(pendingNumber==5)setGatePercent(v>100?100:v);else if(pendingNumber==10){if(!setFlowSetpointLpm((float)v))Serial.println(F("NEXTION ERR: nFlowSetpoint must be 0..100 L/min"));}else if(NEXTION_DEBUG)Serial.println(F("NEXTION warning: number response without pending request"));pendingNumber=0;}
    else if(packetExpected==4&&packet[1]==0xFF&&packet[2]==0xFF&&packet[3]==0xFF){if(NEXTION_DEBUG){Serial.print(F("NEXTION status 0x"));if(packet[0]<16)Serial.print('0');Serial.print(packet[0],HEX);Serial.print(F(": "));Serial.println(nextionStatusName(packet[0]));if(packet[0]==0x1A&&pendingNumber){Serial.print(F("NEXTION ERROR: component/value not found for get "));Serial.println(pendingTarget(pendingNumber));pendingNumber=0;}}}
    else if(NEXTION_DEBUG){Serial.print(F("NEXTION unhandled packet start 0x"));if(packet[0]<16)Serial.print('0');Serial.println(packet[0],HEX);}
    packetUsed=packetExpected=0;
  }
}}
void nextionUiBegin(){NEXTION_SERIAL.begin(NEXTION_BAUD);if(NEXTION_DEBUG){Serial.print(F("NEXTION debug enabled, baud="));Serial.println(NEXTION_BAUD);}}
void nextionUiService(uint32_t now){processInput();if(NEXTION_DEBUG&&pendingNumber&&(uint32_t)(now-pendingSinceMs)>500){Serial.print(F("NEXTION ERROR: no numeric response for get "));Serial.println(pendingTarget(pendingNumber));pendingNumber=0;}for(uint8_t n=0;n<3;++n){uint8_t i=cursor++%(sizeof(fields)/sizeof(fields[0]));char value[32];fieldValue(i,value,sizeof(value),now);sendText(fields[i],value);}}
