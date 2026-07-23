#pragma once
#include <Arduino.h>
void flowMeterBegin();
void flowMeterService(uint32_t now);
bool flowMeterIsCalibrated();
void flowMeterReset();
void flowMeterPrintStatus(bool raw);
