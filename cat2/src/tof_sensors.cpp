#include <Wire.h>
#include <VL53L0X.h>
#include "config.h"
#include "app_state.h"
#include "i2c_mux.h"
#include "tof_sensors.h"

static VL53L0X drivers[TOF_SENSOR_COUNT];
static uint32_t lastRead=0,lastReinit=0;
static uint8_t nextRead=0;
struct ChannelDiag { bool selected,present,supported,initialized; uint8_t model,revision; };
static ChannelDiag diag[I2C_CHANNEL_COUNT];
static bool readReg(uint8_t reg,uint8_t &value){Wire.beginTransmission(0x29);Wire.write(reg);if(Wire.endTransmission(false)!=0)return false;if(Wire.requestFrom((uint8_t)0x29,(uint8_t)1)!=(uint8_t)1)return false;value=Wire.read();return true;}
static bool initializeSlot(uint8_t slot,uint8_t channel){TofState&s=app.tof[slot];s.channel=channel;s.present=true;s.supportedModel=true;s.modelId=diag[channel].model;s.revisionId=diag[channel].revision;if(!i2cMuxSelect(channel))return false;drivers[slot].setTimeout(TOF_TIMEOUT_MS);bool ok=drivers[slot].init();s.initialized=ok;diag[channel].initialized=ok;if(ok){drivers[slot].setMeasurementTimingBudget(TOF_TIMING_BUDGET_US);drivers[slot].startContinuous();}return ok;}
void tofSensorsScan(){
  for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){diag[ch]={false,false,false,false,0,0};diag[ch].selected=i2cMuxSelect(ch);Serial.print(F("ToF channel "));Serial.print(ch);Serial.print(F(": mux="));Serial.print(diag[ch].selected?F("OK"):F("FAIL"));if(!diag[ch].selected){Serial.println();continue;}diag[ch].present=i2cMuxPing(0x29);Serial.print(F(" device_0x29="));Serial.print(diag[ch].present?F("YES"):F("NO"));if(diag[ch].present){readReg(0xC0,diag[ch].model);readReg(0xC2,diag[ch].revision);diag[ch].supported=diag[ch].model==0xEE;Serial.print(F(" model=0x"));Serial.print(diag[ch].model,HEX);Serial.print(F(" rev=0x"));Serial.print(diag[ch].revision,HEX);Serial.print(diag[ch].supported?F(" VL53L0X"):F(" UNSUPPORTED"));}Serial.println();}
  uint8_t slot=0;for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT&&slot<TOF_SENSOR_COUNT;++ch)if(diag[ch].present&&diag[ch].supported){const bool ok=initializeSlot(slot,ch);Serial.print(F("ToF init channel "));Serial.print(ch);Serial.print(F(": "));Serial.println(ok?F("OK"):F("FAILED"));++slot;}for(;slot<TOF_SENSOR_COUNT;++slot)app.tof[slot]={255,0,0,0,false,false,false,false,false,0};
}
void tofSensorsBegin(){tofSensorsScan();}
void tofSensorsService(uint32_t now){
  if((uint32_t)(now-lastRead)<30) return;
  lastRead=now;
  const uint8_t slot=nextRead++%TOF_SENSOR_COUNT; TofState&s=app.tof[slot];
  if(!s.initialized){ if((uint32_t)(now-lastReinit)>TOF_REINIT_MS){lastReinit=now;tofSensorsScan();} return; }
  if(!i2cMuxSelect(s.channel)){s.valid=false;return;}
  uint16_t distance=drivers[slot].readRangeContinuousMillimeters(); s.timeout=drivers[slot].timeoutOccurred();
  if(!s.timeout&&distance!=0&&distance!=65535){s.distanceMm=distance;s.valid=true;s.updatedMs=now;}else s.valid=false;
}
void tofSensorsPrintStatus(){for(uint8_t i=0;i<TOF_SENSOR_COUNT;++i){const TofState&s=app.tof[i];Serial.print(F("ToF "));Serial.print(i+1);Serial.print(F(" ch="));Serial.print(s.channel);Serial.print(F(" model=0x"));Serial.print(s.modelId,HEX);Serial.print(F(" init="));Serial.print(s.initialized?F("yes"):F("no"));Serial.print(F(" valid="));Serial.print(s.valid?F("yes"):F("no"));Serial.print(F(" timeout="));Serial.print(s.timeout?F("yes"):F("no"));if(s.valid){Serial.print(F(" mm="));Serial.print(s.distanceMm);}Serial.println();}}
void tofSensorsReadNow(){lastRead=0;tofSensorsService(millis());tofSensorsPrintStatus();}
