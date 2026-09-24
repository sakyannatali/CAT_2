#include <Arduino.h>
#include "app_state.h"
#include "actuators.h"
#include "i2c_mux.h"
#include "temperature_sensors.h"
#include "tof_sensors.h"
#include "model_control.h"
#include "timer_service.h"
#include "nextion_ui.h"
#include "serial_console.h"

void setup(){serialConsoleBegin();appStateBegin();actuatorsBegin();timerServiceBegin(millis());i2cMuxBegin();temperatureSensorsBegin();tofSensorsBegin();modelControlBegin();nextionUiBegin();Serial.println(F("CAT2 PlatformIO firmware ready"));}
void loop(){const uint32_t now=millis();serialConsoleService();appStateService(now);timerServiceTick(now);temperatureSensorsService(now);tofSensorsService(now);modelControlService(now);nextionUiService(now);}
