#pragma once
#include <Arduino.h>
void piControllerBegin();
void piControllerService(uint32_t now);
void piControllerSetKp(float value);
void piControllerSetTi(float seconds);
float piControllerKp();
float piControllerTi();
void piControllerPrintStatus();
void piControllerPrepareAuto(float currentOutput);
