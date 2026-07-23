#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "app_state.h"
#include "i2c_mux.h"
#include "temperature_sensors.h"

static OneWire oneWire(DS18B20_PIN);
static DallasTemperature ds(&oneWire);
static bool dsPresent=false, conversionPending=false;
static uint32_t conversionStarted=0, lastGyRead=0;
static bool gyPresent[I2C_CHANNEL_COUNT];
static uint8_t gyChannels[2]={255,255};
static uint8_t gyNext=0;

static uint8_t crc8(uint8_t crc,uint8_t data){crc^=data;for(uint8_t i=0;i<8;++i)crc=(crc&0x80)?(uint8_t)((crc<<1)^0x07):(uint8_t)(crc<<1);return crc;}
static bool readMlxWord(uint8_t reg,uint16_t &raw){Wire.beginTransmission(0x5A);Wire.write(reg);if(Wire.endTransmission(false)!=0)return false;if(Wire.requestFrom((uint8_t)0x5A,(uint8_t)3)!=(uint8_t)3)return false;uint8_t l=Wire.read(),h=Wire.read(),pec=Wire.read(),c=0;c=crc8(c,0xB4);c=crc8(c,reg);c=crc8(c,0xB5);c=crc8(c,l);c=crc8(c,h);if(c!=pec)Serial.println(F("GY906 PEC warning"));raw=((uint16_t)h<<8)|l;return true;}
static bool readMlx(float &object){uint16_t raw;if(!readMlxWord(0x07,raw))return false;object=raw*0.02f-273.15f;return object>-100.0f&&object<150.0f;}
void temperatureSensorsScan(){uint8_t found=0;for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){gyPresent[ch]=i2cMuxSelect(ch)&&i2cMuxPing(0x5A);if(gyPresent[ch]){Serial.print(F("GY906 found on mux channel "));Serial.println(ch);if(found<2)gyChannels[found++]=ch;}}while(found<2)gyChannels[found++]=255;}
void temperatureSensorsBegin(){ds.begin();dsPresent=ds.getDeviceCount()>0;ds.setWaitForConversion(false);ds.setResolution(DS18B20_RESOLUTION);Serial.print(F("DS18B20 A10/46: "));Serial.println(dsPresent?F("found"):F("not found"));temperatureSensorsScan();if(dsPresent){ds.requestTemperatures();conversionPending=true;conversionStarted=millis();}}
void temperatureSensorsService(uint32_t now){
  if(dsPresent&&conversionPending&&(uint32_t)(now-conversionStarted)>=DS18B20_CONVERSION_MS){
    float v=ds.getTempCByIndex(0);
    if(v>-100&&v<150&&v!=DEVICE_DISCONNECTED_C) app.outletTemperature={v,true,now}; else app.outletTemperature.valid=false;
    ds.requestTemperatures(); conversionStarted=now;
  }
  if((uint32_t)(now-lastGyRead)<GY906_INTERVAL_MS) return;
  lastGyRead=now;
  for(uint8_t tries=0;tries<2;++tries){
    uint8_t slot=gyNext++%2,ch=gyChannels[slot]; if(ch==255) continue;
    if(!i2cMuxSelect(ch)){app.skinTemperature[slot].valid=false;return;}
    float v; if(readMlx(v))app.skinTemperature[slot]={v,true,now};else app.skinTemperature[slot].valid=false;
    return;
  }
}
void temperatureSensorsPrintStatus(){Serial.print(F("DS18B20: "));if(app.outletTemperature.valid)Serial.println(app.outletTemperature.value,2);else Serial.println(F("ERR"));for(uint8_t i=0;i<2;++i){Serial.print(F("GY906 "));Serial.print(i+1);Serial.print(F(" channel="));Serial.print(gyChannels[i]);Serial.print(F(" value="));if(app.skinTemperature[i].valid)Serial.println(app.skinTemperature[i].value,2);else Serial.println(F("ERR"));}}
