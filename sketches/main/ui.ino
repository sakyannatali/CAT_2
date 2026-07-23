#include "EasyNextionLibrary.h"

EasyNex nex(Serial2);

void setup_nextion() {
  nex.begin(9600);
}

void loop_nextion() {
  nex.NextionListen();

  static unsigned long last_update = 0;
  unsigned long now = millis();

  if (now - last_update >= 1000) {
    last_update = now;
    double ds18b20_temp_c;
    if (ds18b20_get_average(ds18b20_temp_c)) {
      nex.writeNum("fTIn.val", (int)round(ds18b20_temp_c));
    }

    uint8_t gy906_channel;
    float gy906_ambient;
    float gy906_object;

    if (gy906_get_nth_sensor(0, gy906_channel, gy906_ambient, gy906_object)) {
      nex.writeNum("fTSkin1.val", (int)round(gy906_object));
    }
    if (gy906_get_nth_sensor(1, gy906_channel, gy906_ambient, gy906_object)) {
      nex.writeNum("fTSkin2.val", (int)round(gy906_object));
    }

    uint8_t vl53l0x_channel;
    uint16_t vl53l0x_distance;
    uint8_t vl53l0x_status;

    if (vl53l0x_get_nth_sensor(0, vl53l0x_channel, vl53l0x_distance, vl53l0x_status) && vl53l0x_status == 0) {
      nex.writeNum("fLSkin1.val", vl53l0x_distance);
    }
    if (vl53l0x_get_nth_sensor(1, vl53l0x_channel, vl53l0x_distance, vl53l0x_status) && vl53l0x_status == 0) {
      nex.writeNum("fLSkin2.val", vl53l0x_distance);
    }

    double air_flow_volume;
    if (air_flow_get_volume(0, air_flow_volume)) {
      nex.writeNum("fVolume1.val", (int)round(air_flow_volume));
    }
    if (air_flow_get_volume(1, air_flow_volume)) {
      nex.writeNum("fVolume2.val", (int)round(air_flow_volume));
    }

  }
}

void trigger0() {
  Serial.println("COMPRESSOR ON!");
  switch_relay(RELAY_COMPRESSOR, true);
}

void trigger1() {
  Serial.println("COMPRESSOR OFF!");
  switch_relay(RELAY_COMPRESSOR, false);
}

void trigger2() {
  Serial.println("VENT ON!");
  switch_relay(RELAY_VENT, true);
}

void trigger3() {
  Serial.println("VENT OFF!");
  switch_relay(RELAY_VENT, false);
}

void trigger4() {
  Serial.println("VENT POWER CHANGED!");

  int val = nex.readNumber("sVentSpeed.val");
  if (val == 777777) {
    Serial.println("Error reading power number");
    return;
  }
  
  if (val < 0) {
    val = 0;
  }
  if (val > 100) {
    val = 100;
  }
  val = 100 - val;

  int pwm = (val * 255) / 100;
  analogWrite(PWM_VENT, pwm);
  Serial.print("Vent pwm set to ");
  Serial.println(pwm);
}

void trigger5() {
  Serial.println("FLOW DIRECTION CHANGED!");

  int val = nex.readNumber("sGate.val");
  if (val == 777777) {
    Serial.println("Error reading servo number");
    return;
  }
  
  if (val < 0) {
    val = 0;
  }
  if (val > 100) {
    val = 100;
  }
  val = 23 + (val * (70 - 23) / 100);

  // val = 100 - val;

  set_servo(val);
}

void trigger6() {
  Serial.println("TIMER START!");
}

void trigger7() {
  Serial.println("TIMER RESET!");
}

