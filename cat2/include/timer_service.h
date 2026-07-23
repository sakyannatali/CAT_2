#pragma once
#include <Arduino.h>
void timerServiceBegin(uint32_t now);
void timerServiceTick(uint32_t now);
void timerServiceStartOrPause(uint32_t now);
void timerServiceStart(uint32_t now);
void timerServicePause(uint32_t now);
void timerServiceReset(uint32_t now);
void timerServiceFormat(char *out, size_t size);
