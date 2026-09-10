#include <Servo.h>
#include "config.h"
#include "actuators.h"

static Servo gateServo;
static uint8_t relayLevel(bool on) { return (on == RELAY_ACTIVE_LOW) ? LOW : HIGH; }
void actuatorsBegin() {
  pinMode(RELAY_COMPRESSOR_PIN,OUTPUT); pinMode(RELAY_VENT_PIN,OUTPUT); pinMode(FAN_PWM_PIN,OUTPUT);
  digitalWrite(RELAY_COMPRESSOR_PIN,relayLevel(false)); digitalWrite(RELAY_VENT_PIN,relayLevel(false)); analogWrite(FAN_PWM_PIN,FAN_PWM_INVERTED?255:0);
  gateServo.attach(GATE_SERVO_PIN); gateServo.write((GATE_ANGLE_MIN+GATE_ANGLE_MAX+1)/2);
}
void actuatorsApplyCompressor(bool on) { digitalWrite(RELAY_COMPRESSOR_PIN,relayLevel(on)); }
void actuatorsApplyVent(bool on) { digitalWrite(RELAY_VENT_PIN,relayLevel(on)); }
uint8_t actuatorsApplyFan(float percent) { const uint8_t pwm=fanCommandToPwm(percent,FAN_PWM_INVERTED); analogWrite(FAN_PWM_PIN,pwm); return pwm; }
void actuatorsApplyGate(uint8_t angle) { gateServo.write(angle); }
