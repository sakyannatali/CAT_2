#include <Wire.h>
#include "config.h"
#include "app_state.h"
#include "i2c_mux.h"
#include "tof_sensors.h"

static uint32_t lastRead=0;
static uint8_t nextRead=0;
static uint8_t readFailures[TOF_SENSOR_COUNT];
static bool runtimeDisabled[TOF_SENSOR_COUNT];
static bool tofEnabled=false;
struct ChannelDiag { bool selected,present,supported,initialized; uint8_t model,revision; };
static ChannelDiag diag[I2C_CHANNEL_COUNT];
static const uint32_t VL6180X_READY_TIMEOUT_MS = 80;
static const uint8_t VL6180X_MAX_FAILURES = 3;
static const uint16_t VL6180X_IDENTIFICATION_MODEL_ID = 0x0000;
static const uint16_t VL6180X_IDENTIFICATION_REVISION_ID = 0x0002;
static const uint16_t VL6180X_SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0014;
static const uint16_t VL6180X_SYSTEM_INTERRUPT_CLEAR = 0x0015;
static const uint16_t VL6180X_SYSTEM_FRESH_OUT_OF_RESET = 0x0016;
static const uint16_t VL6180X_SYSRANGE_START = 0x0018;
static const uint16_t VL6180X_SYSRANGE_MAX_CONVERGENCE_TIME = 0x001C;
static const uint16_t VL6180X_SYSRANGE_INTERMEASUREMENT_PERIOD = 0x001B;
static const uint16_t VL6180X_RESULT_RANGE_STATUS = 0x004D;
static const uint16_t VL6180X_RESULT_INTERRUPT_STATUS_GPIO = 0x004F;
static const uint16_t VL6180X_RESULT_RANGE_VAL = 0x0062;

static void reportWireTimeout(const __FlashStringHelper *context){
#if defined(WIRE_HAS_TIMEOUT)
  if(Wire.getWireTimeoutFlag()){
    Serial.print(F("I2C TIMEOUT recovered during "));Serial.println(context);
    Wire.clearWireTimeoutFlag();
  }
#else
  (void)context;
#endif
}

static void resetSlot(uint8_t slot){app.tof[slot]={255,0,0,0,false,false,false,false,false,0};readFailures[slot]=0;runtimeDisabled[slot]=true;}

static bool writeReg8(uint16_t reg,uint8_t value){
  Wire.beginTransmission(0x29);
  Wire.write((uint8_t)(reg>>8));
  Wire.write((uint8_t)(reg&0xFF));
  Wire.write(value);
  const bool ok=Wire.endTransmission()==0;
  reportWireTimeout(F("VL6180X write"));
  return ok;
}
static bool readReg8(uint16_t reg,uint8_t &value){
  Wire.beginTransmission(0x29);
  Wire.write((uint8_t)(reg>>8));
  Wire.write((uint8_t)(reg&0xFF));
  if(Wire.endTransmission(false)!=0){reportWireTimeout(F("VL6180X register select"));return false;}
  if(Wire.requestFrom((uint8_t)0x29,(uint8_t)1)!=(uint8_t)1){reportWireTimeout(F("VL6180X read"));return false;}
  value=Wire.read();
  reportWireTimeout(F("VL6180X read"));
  return true;
}

static bool vl6180xLoadSettings(){
  // Minimal VL6180X tuning sequence from ST/Adafruit examples. All writes are bounded by Wire timeout.
  return writeReg8(0x0207,0x01)&&writeReg8(0x0208,0x01)&&writeReg8(0x0096,0x00)&&writeReg8(0x0097,0xFD)&&
         writeReg8(0x00E3,0x00)&&writeReg8(0x00E4,0x04)&&writeReg8(0x00E5,0x02)&&writeReg8(0x00E6,0x01)&&
         writeReg8(0x00E7,0x03)&&writeReg8(0x00F5,0x02)&&writeReg8(0x00D9,0x05)&&writeReg8(0x00DB,0xCE)&&
         writeReg8(0x00DC,0x03)&&writeReg8(0x00DD,0xF8)&&writeReg8(0x009F,0x00)&&writeReg8(0x00A3,0x3C)&&
         writeReg8(0x00B7,0x00)&&writeReg8(0x00BB,0x3C)&&writeReg8(0x00B2,0x09)&&writeReg8(0x00CA,0x09)&&
         writeReg8(0x0198,0x01)&&writeReg8(0x01B0,0x17)&&writeReg8(0x01AD,0x00)&&writeReg8(0x00FF,0x05)&&
         writeReg8(0x0100,0x05)&&writeReg8(0x0199,0x05)&&writeReg8(0x01A6,0x1B)&&writeReg8(0x01AC,0x3E)&&
         writeReg8(0x01A7,0x1F)&&writeReg8(0x0030,0x00)&&writeReg8(VL6180X_SYSRANGE_MAX_CONVERGENCE_TIME,0x32)&&
         writeReg8(VL6180X_SYSRANGE_INTERMEASUREMENT_PERIOD,0x09)&&writeReg8(VL6180X_SYSTEM_INTERRUPT_CONFIG_GPIO,0x24)&&
         writeReg8(VL6180X_SYSTEM_INTERRUPT_CLEAR,0x07)&&writeReg8(VL6180X_SYSTEM_FRESH_OUT_OF_RESET,0x00);
}

static bool initializeSlot(uint8_t slot,uint8_t channel){
  TofState&s=app.tof[slot];
  readFailures[slot]=0;runtimeDisabled[slot]=false;
  s.channel=channel;s.present=true;s.supportedModel=true;s.modelId=diag[channel].model;s.revisionId=diag[channel].revision;
  if(!i2cMuxSelect(channel)){runtimeDisabled[slot]=true;return false;}
  uint8_t model=0;
  if(!readReg8(VL6180X_IDENTIFICATION_MODEL_ID,model)||model!=0xB4){runtimeDisabled[slot]=true;return false;}
  const bool ok=vl6180xLoadSettings();
  s.initialized=ok;diag[channel].initialized=ok;
  if(!ok)runtimeDisabled[slot]=true;
  return ok;
}

static void disableSlot(uint8_t slot,const __FlashStringHelper *reason){
  TofState&s=app.tof[slot];
  Serial.print(F("ToF "));Serial.print(slot+1);Serial.print(F(" ch="));Serial.print(s.channel);Serial.print(F(" disabled: "));Serial.println(reason);
  s.initialized=false;s.valid=false;s.timeout=false;
  runtimeDisabled[slot]=true;
}

static void noteReadResult(uint8_t slot,bool ok){
  if(ok){readFailures[slot]=0;return;}
  if(readFailures[slot]<255)++readFailures[slot];
  Serial.print(F("ToF "));Serial.print(slot+1);Serial.print(F(" failure count="));Serial.println(readFailures[slot]);
  if(readFailures[slot]>=VL6180X_MAX_FAILURES)disableSlot(slot,F("too many read failures; not polling until tof on/scan or reboot"));
}

static bool readRangeSafe(uint8_t &distance,uint8_t &status,uint8_t channel){
  status=0xFF;distance=0;
  if(!writeReg8(VL6180X_SYSRANGE_START,0x01)){Serial.print(F("ToF ch "));Serial.print(channel);Serial.println(F(" range start I2C failed"));return false;}
  const uint32_t started=millis();
  uint8_t irq=0;
  while(true){
    if(!readReg8(VL6180X_RESULT_INTERRUPT_STATUS_GPIO,irq)){Serial.print(F("ToF ch "));Serial.print(channel);Serial.println(F(" ready poll I2C failed"));return false;}
    if(irq&0x04)break;
    if((uint32_t)(millis()-started)>VL6180X_READY_TIMEOUT_MS){Serial.print(F("ToF ch "));Serial.print(channel);Serial.println(F(" measurement ready timeout; loop resumed"));status=1;return false;}
    delay(1);
  }
  if(!readReg8(VL6180X_RESULT_RANGE_VAL,distance)){Serial.print(F("ToF ch "));Serial.print(channel);Serial.println(F(" range read I2C failed"));return false;}
  if(!readReg8(VL6180X_RESULT_RANGE_STATUS,status)){Serial.print(F("ToF ch "));Serial.print(channel);Serial.println(F(" status read I2C failed"));return false;}
  status>>=4;
  if(!writeReg8(VL6180X_SYSTEM_INTERRUPT_CLEAR,0x07)){Serial.print(F("ToF interrupt clear failed on ch "));Serial.println(channel);}
  return true;
}

void tofSensorsSetEnabled(bool enabled){
  tofEnabled=enabled;
  if(!enabled){for(uint8_t i=0;i<TOF_SENSOR_COUNT;++i)resetSlot(i);Serial.println(F("ToF disabled"));return;}
  Serial.println(F("ToF enabled; scanning"));
  tofSensorsScan();
}

bool tofSensorsEnabled(){return tofEnabled;}

void tofSensorsScan(){
  if(!tofEnabled){Serial.println(F("ToF scan skipped: disabled. Use 'tof on' to enable."));for(uint8_t i=0;i<TOF_SENSOR_COUNT;++i)resetSlot(i);return;}
  for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){
    diag[ch]={false,false,false,false,0,0};
    diag[ch].selected=i2cMuxSelect(ch);
    Serial.print(F("ToF channel "));Serial.print(ch);Serial.print(F(": mux="));Serial.print(diag[ch].selected?F("OK"):F("FAIL"));
    if(!diag[ch].selected){Serial.println();continue;}
    diag[ch].present=i2cMuxPing(0x29);
    Serial.print(F(" device_0x29="));Serial.print(diag[ch].present?F("YES"):F("NO"));
    if(diag[ch].present){
      readReg8(VL6180X_IDENTIFICATION_MODEL_ID,diag[ch].model);
      readReg8(VL6180X_IDENTIFICATION_REVISION_ID,diag[ch].revision);
      diag[ch].supported=diag[ch].model==0xB4;
      Serial.print(F(" model=0x"));Serial.print(diag[ch].model,HEX);
      Serial.print(F(" rev=0x"));Serial.print(diag[ch].revision,HEX);
      Serial.print(diag[ch].supported?F(" VL6180X"):F(" UNSUPPORTED"));
    }
    Serial.println();
  }
  uint8_t slot=0;
  for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT&&slot<TOF_SENSOR_COUNT;++ch)if(diag[ch].present&&diag[ch].supported){
    const bool ok=initializeSlot(slot,ch);
    Serial.print(F("ToF init channel "));Serial.print(ch);Serial.print(F(": "));Serial.println(ok?F("OK"):F("FAILED"));
    ++slot;
  }
  for(;slot<TOF_SENSOR_COUNT;++slot)resetSlot(slot);
}

void tofSensorsBegin(){for(uint8_t i=0;i<TOF_SENSOR_COUNT;++i)resetSlot(i);Serial.println(F("ToF disabled by default. Use 'tof on' to enable."));}

void tofSensorsService(uint32_t now){
  if(!tofEnabled)return;
  if((uint32_t)(now-lastRead)<60) return;
  lastRead=now;
  const uint8_t slot=nextRead++%TOF_SENSOR_COUNT; TofState&s=app.tof[slot];
  if(!s.initialized||runtimeDisabled[slot]) return;
  if(!i2cMuxSelect(s.channel)){s.valid=false;reportWireTimeout(F("ToF mux select"));noteReadResult(slot,false);return;}
  uint8_t distance=0,status=0xFF;
  const bool ok=readRangeSafe(distance,status,s.channel);
  s.timeout=!ok&&status==1;
  if(ok&&status==0){s.distanceMm=distance;s.valid=true;s.updatedMs=now;noteReadResult(slot,true);}else{s.valid=false;if(ok){Serial.print(F("ToF ch "));Serial.print(s.channel);Serial.print(F(" range status="));Serial.println(status);}noteReadResult(slot,false);}
}

void tofSensorsPrintStatus(){
  Serial.print(F("ToF enabled="));Serial.println(tofEnabled?F("yes"):F("no"));
  for(uint8_t i=0;i<TOF_SENSOR_COUNT;++i){
    const TofState&s=app.tof[i];
    Serial.print(F("ToF "));Serial.print(i+1);Serial.print(F(" ch="));Serial.print(s.channel);
    Serial.print(F(" model=0x"));Serial.print(s.modelId,HEX);
    Serial.print(F(" init="));Serial.print(s.initialized?F("yes"):F("no"));
    Serial.print(F(" valid="));Serial.print(s.valid?F("yes"):F("no"));
    Serial.print(F(" timeout="));Serial.print(s.timeout?F("yes"):F("no"));
    Serial.print(F(" failures="));Serial.print(readFailures[i]);
    Serial.print(F(" disabled="));Serial.print(runtimeDisabled[i]?F("yes"):F("no"));
    if(s.valid){Serial.print(F(" mm="));Serial.print(s.distanceMm);}
    Serial.println();
  }
}

void tofSensorsReadNow(){if(!tofEnabled){Serial.println(F("ToF read skipped: disabled. Use 'tof on' to enable."));return;}lastRead=0;tofSensorsService(millis());tofSensorsPrintStatus();}
