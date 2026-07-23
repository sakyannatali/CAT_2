#pragma once
#include <Arduino.h>
void actuatorsBegin();
void actuatorsApplyCompressor(bool on);
void actuatorsApplyVent(bool on);
void actuatorsApplyFan(float appliedPercent);
void actuatorsApplyGate(uint8_t angle);
