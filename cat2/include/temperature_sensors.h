#pragma once
#include <Arduino.h>
void temperatureSensorsBegin();
void temperatureSensorsService(uint32_t now);
void temperatureSensorsScan();
void temperatureSensorsPrintStatus();
