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
static uint32_t conversionStarted=0, lastDsRecovery=0, lastGyRead=0;
static uint8_t gyChannels[2]={255,255}, gyNext=0, gyDiscoveryNext=0;
static uint32_t lastGyDiscovery[2]={0,0};
static const char *dsLastError="NOT READ";
static const char *gyLastError[2]={"NOT READ","NOT READ"};

enum MlxReadResult : uint8_t { MLX_OK, MLX_TX_ERROR, MLX_RX_ERROR, MLX_TIMEOUT, MLX_PEC_ERROR, MLX_RANGE_ERROR };

static uint8_t crc8(uint8_t crc,uint8_t data){crc^=data;for(uint8_t i=0;i<8;++i)crc=(crc&0x80)?(uint8_t)((crc<<1)^0x07):(uint8_t)(crc<<1);return crc;}
static const char *mlxResultText(MlxReadResult result){
  switch(result){
    case MLX_OK:return "NONE";
    case MLX_TX_ERROR:return "I2C TX";
    case MLX_RX_ERROR:return "I2C RX";
    case MLX_TIMEOUT:return "I2C TIMEOUT";
    case MLX_PEC_ERROR:return "PEC";
    default:return "RANGE";
  }
}
static MlxReadResult readMlxWord(uint8_t reg,uint16_t &raw){
  Wire.beginTransmission(0x5A);
  Wire.write(reg);
  const uint8_t tx=Wire.endTransmission(false);
  if(i2cMuxWireTimedOut(F("GY906 register select")))return MLX_TIMEOUT;
  if(tx!=0)return MLX_TX_ERROR;
  const uint8_t received=Wire.requestFrom((uint8_t)0x5A,(uint8_t)3);
  if(i2cMuxWireTimedOut(F("GY906 read")))return MLX_TIMEOUT;
  if(received!=3||Wire.available()<3)return MLX_RX_ERROR;
  const uint8_t l=Wire.read(),h=Wire.read(),pec=Wire.read();
  uint8_t c=0;c=crc8(c,0xB4);c=crc8(c,reg);c=crc8(c,0xB5);c=crc8(c,l);c=crc8(c,h);
  if(c!=pec)return MLX_PEC_ERROR;
  raw=((uint16_t)h<<8)|l;
  return MLX_OK;
}
static MlxReadResult readMlx(float &object){
  uint16_t raw=0;
  const MlxReadResult result=readMlxWord(0x07,raw);
  if(result!=MLX_OK)return result;
  object=raw*0.02f-273.15f;
  return object>-100.0f&&object<150.0f ? MLX_OK : MLX_RANGE_ERROR;
}
static bool configureDs(){
  ds.begin();
  if(ds.getDeviceCount()==0)return false;
  ds.setWaitForConversion(false);
  ds.setResolution(DS18B20_RESOLUTION);
  return true;
}
static void requestDsConversion(uint32_t now){
  if(!dsPresent)return;
  ds.requestTemperatures();
  conversionPending=true;
  conversionStarted=now;
}
static bool channelAssigned(uint8_t channel){return gyChannels[0]==channel||gyChannels[1]==channel;}
static void recordTemperatureSuccess(TemperatureFaultState &state,float value,uint32_t now,const __FlashStringHelper *name){
  const bool wasStale=state.stale;
  temperatureStateRecordSuccess(state,value,now);
  if(wasStale){Serial.print(name);Serial.println(F(" temperature recovered"));}
}

void temperatureSensorsScan(){
  uint8_t found=0;
  gyChannels[0]=gyChannels[1]=255;
  for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){
    if(!i2cMuxSelect(ch)||!i2cMuxPing(0x5A))continue;
    Serial.print(F("GY906 found on mux channel "));Serial.println(ch);
    if(found<2)gyChannels[found++]=ch;
  }
}
void temperatureSensorsBegin(){
  const uint32_t now=millis();
  dsPresent=configureDs();
  Serial.print(F("DS18B20 A10/46: "));Serial.println(dsPresent?F("found"):F("not found"));
  if(dsPresent){dsLastError="NONE";requestDsConversion(now);}
  else {dsLastError="NOT FOUND";temperatureStateRecordFailure(app.outletTemperature,now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);lastDsRecovery=now;}
  temperatureSensorsScan();
}
static void serviceDs(uint32_t now){
  temperatureStateRefresh(app.outletTemperature,now,TEMP_STALE_TIMEOUT_MS);
  if(dsPresent&&conversionPending&&intervalElapsed(now,conversionStarted,DS18B20_CONVERSION_MS)){
    const float value=ds.getTempCByIndex(0);
    if(value>-100.0f&&value<150.0f&&value!=DEVICE_DISCONNECTED_C){
      recordTemperatureSuccess(app.outletTemperature,value,now,F("DS18B20"));
      dsLastError="NONE";
      requestDsConversion(now);
    }else{
      dsLastError="DS READ";
      temperatureStateRecordFailure(app.outletTemperature,now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);
      if(app.outletTemperature.stale){dsPresent=false;conversionPending=false;lastDsRecovery=now;}
      else requestDsConversion(now);
    }
  }
  if(!dsPresent&&intervalElapsed(now,lastDsRecovery,TEMP_REINIT_INTERVAL_MS)){
    lastDsRecovery=now;
    dsPresent=configureDs();
    if(dsPresent){dsLastError="NONE";requestDsConversion(now);}
    else {dsLastError="NOT FOUND";temperatureStateRecordFailure(app.outletTemperature,now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);}
  }
}
static void discoverGyChannel(uint8_t slot,uint32_t now){
  if(gyChannels[slot]!=255||!intervalElapsed(now,lastGyDiscovery[slot],TEMP_REINIT_INTERVAL_MS))return;
  lastGyDiscovery[slot]=now;
  const uint8_t channel=gyDiscoveryNext++%I2C_CHANNEL_COUNT;
  if(channelAssigned(channel))return;
  if(i2cMuxSelect(channel)&&i2cMuxPing(0x5A)){
    gyChannels[slot]=channel;
    gyLastError[slot]="NONE";
    Serial.print(F("GY906 rediscovered on mux channel "));Serial.println(channel);
    return;
  }
  gyLastError[slot]="NOT FOUND";
  temperatureStateRecordFailure(app.skinTemperature[slot],now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);
}
static void serviceGySlot(uint8_t slot,uint32_t now){
  TemperatureFaultState &state=app.skinTemperature[slot];
  temperatureStateRefresh(state,now,TEMP_STALE_TIMEOUT_MS);
  discoverGyChannel(slot,now);
  const uint8_t channel=gyChannels[slot];
  if(channel==255)return;
  if(!i2cMuxSelect(channel)){
    gyLastError[slot]="MUX SELECT";
    temperatureStateRecordFailure(state,now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);
    return;
  }
  float value=NAN;
  const MlxReadResult result=readMlx(value);
  gyLastError[slot]=mlxResultText(result);
  if(result==MLX_OK)recordTemperatureSuccess(state,value,now,slot==0?F("GY906 1"):F("GY906 2"));
  else temperatureStateRecordFailure(state,now,TEMP_FAIL_COUNT_LIMIT,TEMP_STALE_TIMEOUT_MS);
}
void temperatureSensorsService(uint32_t now){
  serviceDs(now);
  for(uint8_t i=0;i<2;++i)temperatureStateRefresh(app.skinTemperature[i],now,TEMP_STALE_TIMEOUT_MS);
  if(!intervalElapsed(now,lastGyRead,GY906_INTERVAL_MS))return;
  lastGyRead=now;
  serviceGySlot(gyNext++%2,now);
}
static void printTemperatureState(const __FlashStringHelper *name,const TemperatureFaultState &state,const char *error,uint32_t now){
  Serial.print(name);Serial.print(F(": value="));
  if(state.hasLastGood)Serial.print(state.value,2);else Serial.print(F("ERR"));
  Serial.print(F(" valid="));Serial.print(temperatureStateUsable(state,now,TEMP_STALE_TIMEOUT_MS)?F("yes"):F("no"));
  Serial.print(F(" stale="));Serial.print(state.stale?F("yes"):F("no"));
  Serial.print(F(" fail_count="));Serial.print(state.failCount);
  Serial.print(F(" age_ms="));
  if(state.hasLastGood)Serial.print(temperatureStateAgeMs(state,now));else Serial.print(F("never"));
  Serial.print(F(" last_error="));Serial.println(error);
}
void temperatureSensorsPrintStatus(){
  const uint32_t now=millis();
  printTemperatureState(F("DS18B20"),app.outletTemperature,dsLastError,now);
  for(uint8_t i=0;i<2;++i){
    Serial.print(F("GY906 "));Serial.print(i+1);Serial.print(F(" channel="));Serial.println(gyChannels[i]);
    printTemperatureState(i==0?F("GY906 1"):F("GY906 2"),app.skinTemperature[i],gyLastError[i],now);
  }
  i2cMuxPrintDiagnostics();
}
