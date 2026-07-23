#include <Arduino.h>
#include "app_state.h"
#include "actuators.h"
#include "flow_meter.h"
#include "i2c_mux.h"
#include "temperature_sensors.h"
#include "tof_sensors.h"
#include "pi_controller.h"
#include "timer_service.h"
#include "nextion_ui.h"
#include "serial_console.h"

void setup(){serialConsoleBegin();appStateBegin();actuatorsBegin();timerServiceBegin(millis());i2cMuxBegin();temperatureSensorsBegin();tofSensorsBegin();flowMeterBegin();piControllerBegin();nextionUiBegin();Serial.println(F("CAT2 PlatformIO firmware ready"));}
void loop(){const uint32_t now=millis();serialConsoleService();timerServiceTick(now);flowMeterService(now);temperatureSensorsService(now);tofSensorsService(now);piControllerService(now);nextionUiService(now);}
