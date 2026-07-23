#include <Wire.h>
#include "config.h"
#include "i2c_mux.h"
void i2cMuxBegin(){ Wire.begin(); Wire.setClock(I2C_CLOCK_HZ); Serial.print(F("I2C mux 0x70: "));Serial.println(i2cMuxPing(I2C_MUX_ADDRESS)?F("ACK"):F("NO ACK")); }
bool i2cMuxPing(uint8_t address){Wire.beginTransmission(address);return Wire.endTransmission()==0;}
bool i2cMuxSelect(uint8_t channel){if(channel>=I2C_CHANNEL_COUNT)return false;Wire.beginTransmission(I2C_MUX_ADDRESS);Wire.write((uint8_t)(0x08|channel));const bool ok=Wire.endTransmission()==0;if(ok)delayMicroseconds(I2C_MUX_SETTLE_US);return ok;}
void i2cMuxScanBus(){Serial.println(F("I2C raw scan:"));for(uint8_t a=1;a<127;++a)if(i2cMuxPing(a)){Serial.print(F("  0x"));if(a<16)Serial.print('0');Serial.println(a,HEX);}}
void i2cMuxScanChannels(){for(uint8_t ch=0;ch<I2C_CHANNEL_COUNT;++ch){Serial.print(F("MUX channel "));Serial.print(ch);if(!i2cMuxSelect(ch)){Serial.println(F(": select failed"));continue;}Serial.println(':');bool found=false;for(uint8_t a=1;a<127;++a)if(a!=I2C_MUX_ADDRESS&&i2cMuxPing(a)){found=true;Serial.print(F("  0x"));if(a<16)Serial.print('0');Serial.println(a,HEX);}if(!found)Serial.println(F("  no devices"));}}
