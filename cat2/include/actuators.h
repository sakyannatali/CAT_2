#pragma once
#include <Arduino.h>
void actuatorsBegin();
void actuatorsApplyCompressor(bool on);
void actuatorsApplyVent(bool on);
uint8_t actuatorsApplyFan(float appliedPercent);
void actuatorsApplyGate(uint8_t angle);
