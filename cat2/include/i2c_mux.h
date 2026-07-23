#pragma once
#include <Arduino.h>
void i2cMuxBegin();
bool i2cMuxSelect(uint8_t channel);
bool i2cMuxPing(uint8_t address);
void i2cMuxScanBus();
void i2cMuxScanChannels();
