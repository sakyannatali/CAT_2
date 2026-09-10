#pragma once

#include <Arduino.h>
#include "pure_logic.h"

// Hardware pins. Values preserve the currently working sketches/main wiring.
static const uint8_t RELAY_COMPRESSOR_PIN = 43;
static const uint8_t RELAY_VENT_PIN = 42;
static const uint8_t FAN_PWM_PIN = 9;
static const uint8_t GATE_SERVO_PIN = 45;
static const uint8_t DS18B20_PIN = A10;  // A10 is digital pin 64 on Mega.

// README formerly said DS18B20 was on A10; sketches/main confirms it is A10/46.
// Do not change this pin without checking the installed harness.

static const bool RELAY_ACTIVE_LOW = true;
static const bool FAN_PWM_INVERTED = true;  // Old UI inverted PWM; one authority now.
static const uint8_t GATE_ANGLE_MIN = 23;
static const uint8_t GATE_ANGLE_MAX = 70;

static const uint8_t FLOW_SENSOR_COUNT = 2;
static const uint8_t FLOW_PINS[FLOW_SENSOR_COUNT] = {2, 3};
static const int FLOW_INTERRUPT_MODE = FALLING;  // NPN with external pull-up.
static const uint32_t FLOW_MIN_PULSE_PERIOD_US = 1200UL;
static const uint32_t FLOW_ZERO_TIMEOUT_MS = 3000UL;
static const uint32_t FLOW_UPDATE_INTERVAL_MS = 250UL;
// The experimentally measured transfer functions are built into pure_logic.h.
// Air-density correction comes from include/generated/air_density_table.h.
static const bool FLOW_CONVERSION_CONFIGURED = true;
static const uint32_t FLOW_FILTER_WINDOW_MS = 5000UL;
static const uint32_t FLOW_FILTER_SAMPLE_INTERVAL_MS = 500UL;
static const uint32_t FLOW_DISPLAY_UPDATE_MS = 5000UL;
static const uint32_t FLOW_OUTLET_TEMPERATURE_HOLD_MS = 30000UL;
static const float FLOW_SETPOINT_MIN_LPM = 0.0f;
static const float FLOW_SETPOINT_MAX_LPM = 100.0f;
static const uint32_t EDIT_APPLY_TIMEOUT_MS = 5000UL;

static const uint8_t I2C_MUX_ADDRESS = 0x70;
static const uint32_t I2C_CLOCK_HZ = 50000UL;
static const uint16_t I2C_MUX_SETTLE_US = 250;
static const uint8_t I2C_CHANNEL_COUNT = 8;
static const uint8_t TOF_SENSOR_COUNT = 2;
static const uint16_t TOF_TIMEOUT_MS = 50;
static const uint32_t TOF_TIMING_BUDGET_US = 20000UL;
static const uint32_t TOF_STALE_MS = 1500UL;
static const uint32_t TOF_REINIT_MS = 10000UL;

static const uint8_t DS18B20_RESOLUTION = 12;
static const uint32_t DS18B20_CONVERSION_MS = 800UL;
static const uint32_t TEMPERATURE_STALE_MS = 5000UL;
static const uint32_t GY906_INTERVAL_MS = 500UL;

// Set to 1 only after recompiling the HMI and moving its cable to Serial1.
#define NEXTION_USE_SERIAL1 0
#if NEXTION_USE_SERIAL1
  #define NEXTION_SERIAL Serial1
#else
  #define NEXTION_SERIAL Serial2
#endif
static const uint32_t NEXTION_BAUD = 9600UL;
static const uint32_t NEXTION_FULL_SYNC_MS = 1000UL;

static const float PI_DEFAULT_KP = 0.5f;
static const float PI_DEFAULT_TI_S = 100.0f;
static const float PI_MIN_NORMALIZATION_LPM = 1.0f;
static const float PI_MAX_OUTPUT_STEP_PER_S = 5.0f;
// With Kp=0.5 and Ti=100 s this range permits bumpless transfer up to 100%.
static const float PI_INTEGRAL_LIMIT = 20000.0f;
static const uint32_t PI_UPDATE_INTERVAL_MS = 1000UL;
// 0 = meter 1, 1 = meter 2, 2 = average of valid meters.
static const uint8_t PI_DEFAULT_FEEDBACK_SOURCE = 2;
