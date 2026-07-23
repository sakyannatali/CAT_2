#pragma once
#include <Arduino.h>
void tofSensorsBegin();
void tofSensorsService(uint32_t now);
void tofSensorsScan();
void tofSensorsPrintStatus();
void tofSensorsReadNow();
