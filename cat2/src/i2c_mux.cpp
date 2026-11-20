#include <Wire.h>
#include "config.h"
#include "i2c_mux.h"
static uint16_t wireTimeoutCount=0, wireRecoverCount=0;
static bool lastRecoverOk=true;

static void configureWire(){
  Wire.begin();
  Wire.setClock(I2C_CLOCK_HZ);
  // Arduino AVR Core in this project exposes these methods even though it does
  // not define WIRE_HAS_TIMEOUT. Resetting TWI prevents a held bus from
  // permanently stalling subsequent transactions.
  Wire.setWireTimeout(I2C_WIRE_TIMEOUT_US,true);
  Wire.clearWireTimeoutFlag();
}
static bool recoverWire(){
  if(wireRecoverCount<UINT16_MAX)++wireRecoverCount;
  configureWire();
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  const bool ack=Wire.endTransmission()==0;
  const bool timedOut=Wire.getWireTimeoutFlag();
  if(timedOut)Wire.clearWireTimeoutFlag();
  lastRecoverOk=ack&&!timedOut;
  return lastRecoverOk;
}
bool i2cMuxWireTimedOut(const __FlashStringHelper *context){
  if(!Wire.getWireTimeoutFlag())return false;
  Wire.clearWireTimeoutFlag();
  if(wireTimeoutCount<UINT16_MAX)++wireTimeoutCount;
  Serial.print(F("I2C timeout during "));Serial.println(context);
  const bool recovered=recoverWire();
  Serial.print(F("I2C recovery: "));Serial.println(recovered?F("MUX ACK"):F("MUX NO ACK"));
  return true;
}
void i2cMuxBegin(){ configureWire();
Serial.print(F("I2C Wire timeout enabled: "));Serial.print(I2C_WIRE_TIMEOUT_US/1000UL);Serial.println(F(" ms, reset on timeout"));
Serial.print(F("I2C mux 0x70: "));Serial.println(i2cMuxPing(I2C_MUX_ADDRESS)?F("ACK"):F("NO ACK")); }
bool i2cMuxPing(uint8_t address){Wire.beginTransmission(address);const bool ok=Wire.endTransmission()==0;const bool timeout=i2cMuxWireTimedOut(F("I2C ping"));return ok&&!timeout;}
bool i2cMuxSelect(uint8_t channel){if(channel>=I2C_CHANNEL_COUNT)return false;Wire.beginTransmission(I2C_MUX_ADDRESS);Wire.write((uint8_t)(0x08|channel));const bool ok=Wire.endTransmission()==0;const bool timeout=i2cMuxWireTimedOut(F("I2C mux select"));if(ok&&!timeout)delayMicroseconds(I2C_MUX_SETTLE_US);return ok&&!timeout;}
void i2cMuxPrintDiagnostics(){
  Serial.print(F("I2C timeout_count="));Serial.print(wireTimeoutCount);
  Serial.print(F(" recover_count="));Serial.print(wireRecoverCount);
  Serial.print(F(" last_recover="));Serial.println(lastRecoverOk?F("MUX ACK"):F("MUX NO ACK"));
}
void i2cMuxScanBus(){Serial.println(F("I2C raw scan:"));for(uint8_t a=1;a<127;++a)if(i2cMuxPing(a)){Serial.print(F("  0x"));if(a<16)Serial.print('0');Serial.println(a,HEX);}}
void i2cMuxScanChannels(){for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){Serial.print(F("MUX channel "));Serial.print(ch);if(!i2cMuxSelect(ch)){Serial.println(F(": select failed"));continue;}Serial.println(':');bool found=false;for(uint8_t a=1;a<127;++a)if(a!=I2C_MUX_ADDRESS&&i2cMuxPing(a)){found=true;Serial.print(F("  0x"));if(a<16)Serial.print('0');Serial.println(a,HEX);}if(!found)Serial.println(F("  no devices"));}}
