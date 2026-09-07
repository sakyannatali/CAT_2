#pragma once
#include <Arduino.h>
void flowMeterBegin();
void flowMeterService(uint32_t now);
bool flowMeterConversionConfigured();
const char *flowMeterAutoBlockReason();
void flowMeterReset();
void flowMeterPrintStatus(bool raw);
void flowMeterPrintDensity();
