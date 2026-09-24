#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#endif
#include "pure_logic.h"

// Hardware pins. Values preserve the currently working sketches/main wiring.
static const uint8_t RELAY_COMPRESSOR_PIN = 43;
static const uint8_t RELAY_VENT_PIN = 42;
static const uint8_t FAN_PWM_PIN = 9;
static const uint8_t GATE_SERVO_PIN = 45;
#ifdef ARDUINO
static const uint8_t DS18B20_PIN = A10;  // A10 is digital pin 64 on Mega.
#else
static const uint8_t DS18B20_PIN = 64;   // Native tests have no Arduino A10 macro.
#endif

// README formerly said DS18B20 was on A10; sketches/main confirms it is A10/46.
// Do not change this pin without checking the installed harness.

static const bool RELAY_ACTIVE_LOW = true;
static const bool FAN_PWM_INVERTED = true;  // Old UI inverted PWM; one authority now.
static const uint8_t GATE_ANGLE_MIN = 23;
static const uint8_t GATE_ANGLE_MAX = 70;

// D2 and D3 are deliberately not repurposed. The former pulse flow meters are
// physically disconnected; flow is now an open-loop experimental model.
static const float FLOW_MODEL_LPM_PER_MPS = 7.363107781851078f; // D=12.5 mm.
static const float FLOW_MODEL_CALIBRATED_MIN_POWER_PERCENT = 5.0f;
static const float FLOW_MODEL_CALIBRATED_MAX_POWER_PERCENT = 60.0f;
static const float FLOW_MODEL_NEG100_OUTPUT1_A = 37.4715298098f;
static const float FLOW_MODEL_NEG100_OUTPUT1_K = 0.031191095466f;
static const float FLOW_MODEL_NEG100_OUTPUT2_A = 7.2859015379f;
static const float FLOW_MODEL_NEG100_OUTPUT2_K = 0.027189607669f;
static const float FLOW_MODEL_NEG50_OUTPUT1_A = 35.9505472343f;
static const float FLOW_MODEL_NEG50_OUTPUT1_K = 0.033407197734f;
static const float FLOW_MODEL_NEG50_OUTPUT2_A = 35.5694315657f;
static const float FLOW_MODEL_NEG50_OUTPUT2_K = 0.017748157991f;
static const float FLOW_MODEL_ZERO_OUTPUT1_A = 37.5924334733f;
static const float FLOW_MODEL_ZERO_OUTPUT1_K = 0.025236134022f;
static const float FLOW_MODEL_ZERO_OUTPUT2_A = 36.8919894293f;
static const float FLOW_MODEL_ZERO_OUTPUT2_K = 0.027292152338f;
static const float FLOW_SETPOINT_MIN_LPM = 30.0f;
// This is a user-target limit, independent from the flow predicted at 100%
// fan power. Requests above a model's physical maximum remain UNREACHABLE.
static const float FLOW_SETPOINT_MAX_LPM = 500.0f;
static const uint32_t EDIT_APPLY_TIMEOUT_MS = 5000UL;
static const float FLOW_SETPOINT_EDIT_STEP_LPM = 5.0f;
static const float GATE_COMMAND_MIN_PERCENT = -100.0f;
static const float GATE_COMMAND_MAX_PERCENT = 100.0f;
static const float GATE_COMMAND_EDIT_STEP_PERCENT = 10.0f;

static const uint8_t I2C_MUX_ADDRESS = 0x70;
static const uint32_t I2C_CLOCK_HZ = 50000UL;
static const uint32_t I2C_WIRE_TIMEOUT_US = 50000UL;
static const uint16_t I2C_MUX_SETTLE_US = 250;
static const uint8_t I2C_CHANNEL_COUNT = 8;
static const uint8_t TOF_SENSOR_COUNT = 2;
static const uint16_t TOF_TIMEOUT_MS = 50;
static const uint32_t TOF_TIMING_BUDGET_US = 20000UL;
static const uint32_t TOF_STALE_MS = 1500UL;
static const uint32_t TOF_REINIT_MS = 10000UL;

static const uint8_t DS18B20_RESOLUTION = 12;
static const uint32_t DS18B20_CONVERSION_MS = 800UL;
static const uint8_t TEMP_FAIL_COUNT_LIMIT = 3;
static const uint32_t TEMP_STALE_TIMEOUT_MS = 2500UL;
static const uint32_t TEMP_REINIT_INTERVAL_MS = 1000UL;
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
