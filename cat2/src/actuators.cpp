#include <Servo.h>
#include "config.h"
#include "actuators.h"

static Servo gateServo;
static uint8_t relayLevel(bool on) { return (on == RELAY_ACTIVE_LOW) ? LOW : HIGH; }
void actuatorsBegin() {
  pinMode(RELAY_COMPRESSOR_PIN,OUTPUT); pinMode(RELAY_VENT_PIN,OUTPUT); pinMode(FAN_PWM_PIN,OUTPUT);
  digitalWrite(RELAY_COMPRESSOR_PIN,relayLevel(false)); digitalWrite(RELAY_VENT_PIN,relayLevel(false)); analogWrite(FAN_PWM_PIN,FAN_PWM_INVERTED?255:0);
  gateServo.attach(GATE_SERVO_PIN); gateServo.write(GATE_ANGLE_MIN);
}
void actuatorsApplyCompressor(bool on) { digitalWrite(RELAY_COMPRESSOR_PIN,relayLevel(on)); }
void actuatorsApplyVent(bool on) { digitalWrite(RELAY_VENT_PIN,relayLevel(on)); }
void actuatorsApplyFan(float percent) { if(percent<0)percent=0; if(percent>100)percent=100; uint8_t pwm=(uint8_t)(percent*255.0f/100.0f+0.5f); if(FAN_PWM_INVERTED)pwm=255-pwm; analogWrite(FAN_PWM_PIN,pwm); }
void actuatorsApplyGate(uint8_t angle) { gateServo.write(angle); }
