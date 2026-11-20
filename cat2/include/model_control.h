#pragma once

#include <stdint.h>

// Open-loop feed-forward controller. It has no flow-meter or PI dependency.
void modelControlBegin();
void modelControlService(uint32_t now);
void modelControlRequestAutoUpdate();
void modelControlPrintStatus();
