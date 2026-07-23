#include <Adafruit_MAX31865.h>
#include <Wire.h>
#include <Servo.h>
#include <SoftWire.h>


#define SDA_PIN 21
#define SCL_PIN 20


static const int RELAY_COMPRESSOR = 43;
static const int RELAY_VENT = 42;
static const int PWM_VENT = 9;
static const int ONE_WIRE_BUS = 46;

Servo servo;
static const int SERVO = 45;

static const int RELAY_OFF = HIGH;
static const int RELAY_ON = LOW;


void setup_hardware() {
  // I2C
  setup_soft_i2c_hub();
  scan_soft_i2c_bus();
  setup_gy906();
  setup_vl53l0x();

  // 1-WIRE
  setup_ds18b20();

  // Analog air flow sensors
  setup_air_flow();

  // Relays
  pinMode(RELAY_VENT, OUTPUT);
  pinMode(RELAY_COMPRESSOR, OUTPUT);
  
  digitalWrite(RELAY_VENT, RELAY_OFF);
  digitalWrite(RELAY_COMPRESSOR, RELAY_OFF);

  // PWM
  servo.attach(SERVO);
  set_servo(46);
  pinMode(PWM_VENT, OUTPUT);

  // Help output
  print_help();
}


void loop_hardware() {
  // serial_control_fetch();

  loop_air_flow();
  loop_ds18b20();
  loop_gy906();
  loop_vl53l0x();
}



void serial_control_fetch() {
  static const size_t SERIAL_CONTROL_BUF_SIZE = 32;
  static char buf[SERIAL_CONTROL_BUF_SIZE];
  static uint8_t n = 0;

  while (Serial.available()) {
    char c = (char)Serial.read();

    if (n >= SERIAL_CONTROL_BUF_SIZE) {
      n = 0;
      Serial.println("WARNING: serial control buffer overflow!");
    }

    if (c == '\r' || c == '\n') {
      buf[n] = 0;
      if (n) handleLine(buf);
      n = 0;
    } else if (n < SERIAL_CONTROL_BUF_SIZE) {
      buf[n] = c;
      n += 1;
    }
  }
}


void print_help() {
  Serial.println("Available commands:");
  Serial.println(" - vent on/off/%");
  Serial.println(" - servo %");
  Serial.println(" - comp on/off");
  Serial.println(" - scan");
  Serial.println(" - scan-soft");
  Serial.println(" - scan-soft-bus");
  Serial.println(" - temp");
  Serial.println(" - vl53");
  Serial.println(" - air");
  Serial.println("");
}


void scan_i2c() {
  int nDevices = 0;

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; ++address) {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");

      ++nDevices;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
}


char *skip_init(char *s) {
  while (*s && *s != ' ' && *s != '\t') s++;
  while (*s == ' ' || *s == '\t') s++;
  return s;
}


void handleLine(char *s) {
  while (*s == ' ' || *s == '\t') s++;
  if (!*s) return;

  if (strncmp(s, "help", 4) == 0 || strncmp(s, "?", 1) == 0) {
    print_help();
  } else if (strncmp(s, "comp", 4) == 0) {
    handle_comp(skip_init(s));
  } else if (strncmp(s, "vent", 4) == 0) {
    handle_vent(skip_init(s));
  } else if (strncmp(s, "servo", 5) == 0) {
    handle_servo(skip_init(s));
  } else if (strncmp(s, "scan-soft-bus", 13) == 0) {
    scan_soft_i2c_bus();
  } else if (strncmp(s, "scan-soft", 9) == 0) {
    scan_soft_i2c_hub();
  } else if (strncmp(s, "scan", 4) == 0) {
    scan_i2c();
  } else if (strncmp(s, "temp", 4) == 0) {
    fetch_temp();
  } else if (strncmp(s, "vl53", 4) == 0) {
    fetch_vl53l0x();
  } else if (strncmp(s, "air", 3) == 0) {
    fetch_air_flow();
  } else {
    Serial.println(F("ERR: no such device"));
  }
}


bool handle_relay(char *s, uint8_t pin) {
  bool on;
  if (strncmp(s, "on", 2) == 0) on = true;
  else if (strncmp(s, "off", 3) == 0) on = false;
  else return false;

  switch_relay(pin, on);

  return true;
}


void switch_relay(uint8_t pin, bool on) {
  if (pin == RELAY_VENT) {
    Serial.print(F("Vent Relay: "));
  } else if (pin == RELAY_COMPRESSOR) {
    Serial.print(F("Compressor Relay: "));
  } else {
    Serial.print(F("Unhandled Relay, nothing is done"));
    return;
  }
  digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);
  Serial.println(on ? F("on") : F("off"));
}


void handle_comp(char *s) {
  bool handled = handle_relay(s, RELAY_COMPRESSOR);
  if (handled) return;
}


void handle_vent(char *s) {
  bool handled = handle_relay(s, RELAY_VENT);
  if (handled) {
    Serial.println(F("Vent Relay switched"));
    return;
  }

  int pct = atoi(s);
  Serial.print(F("Vent value: "));
  Serial.println(pct);

  int pwm = (pct * 255) / 100;
  analogWrite(PWM_VENT, pwm);
}


void handle_servo(char *s) {
  int pct = atoi(s);
  Serial.print(F("Servo pct: "));
  Serial.println(pct);

  set_servo(pct);
}


void set_servo(int angle) {
  servo.write(angle);
  Serial.print(F("Angle written: "));
  Serial.println(angle);
}


