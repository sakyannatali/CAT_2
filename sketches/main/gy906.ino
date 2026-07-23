#include <SoftWire.h>

extern SoftWire soft_i2c;

static const uint8_t GY906_ADDR = 0x5A;       // GY-906 / MLX90614 default address
static const uint8_t GY906_REG_TA = 0x06;     // Ambient temperature
static const uint8_t GY906_REG_TOBJ = 0x07;   // Object temperature
static const uint8_t GY906_CHANNEL_COUNT = 8;

bool gy906_sensor_on_channel[GY906_CHANNEL_COUNT];
float gy906_ambient_c[GY906_CHANNEL_COUNT];
float gy906_object_c[GY906_CHANNEL_COUNT];
unsigned long gy906_last_read_ms[GY906_CHANNEL_COUNT];

uint8_t gy906_crc8_update(uint8_t crc, uint8_t data) {
  crc ^= data;

  for (uint8_t i = 0; i < 8; i++) {
    if (crc & 0x80) {
      crc = (crc << 1) ^ 0x07;
    } else {
      crc <<= 1;
    }
  }

  return crc;
}

bool gy906_check_pec(uint8_t addr, uint8_t reg, uint8_t lsb, uint8_t msb, uint8_t pec) {
  uint8_t crc = 0;

  crc = gy906_crc8_update(crc, (addr << 1) | 0);  // write address
  crc = gy906_crc8_update(crc, reg);
  crc = gy906_crc8_update(crc, (addr << 1) | 1);  // read address
  crc = gy906_crc8_update(crc, lsb);
  crc = gy906_crc8_update(crc, msb);

  return crc == pec;
}

bool read_gy906_raw(uint8_t reg, uint16_t &raw) {
  soft_i2c.beginTransmission(GY906_ADDR);
  soft_i2c.write(reg);

  // false = repeated start, required for SMBus read word.
  uint8_t err = soft_i2c.endTransmission(false);
  if (err != 0) {
    return false;
  }

  uint8_t count = soft_i2c.requestFrom((uint8_t)GY906_ADDR, (uint8_t)3, (uint8_t)true);
  if (count != 3) {
    return false;
  }

  uint8_t lsb = soft_i2c.read();
  uint8_t msb = soft_i2c.read();
  uint8_t pec = soft_i2c.read();

  if (!gy906_check_pec(GY906_ADDR, reg, lsb, msb, pec)) {
    Serial.println(F("GY-906 warning: PEC check failed"));
  }

  raw = ((uint16_t)msb << 8) | lsb;
  return true;
}

float gy906_raw_to_celsius(uint16_t raw) {
  return raw * 0.02 - 273.15;
}

bool read_gy906(float &ambientC, float &objectC) {
  uint16_t rawAmbient;
  uint16_t rawObject;

  if (!read_gy906_raw(GY906_REG_TA, rawAmbient)) {
    return false;
  }

  if (!read_gy906_raw(GY906_REG_TOBJ, rawObject)) {
    return false;
  }

  ambientC = gy906_raw_to_celsius(rawAmbient);
  objectC = gy906_raw_to_celsius(rawObject);
  return true;
}

void find_gy906_sensors_on_all_channels() {
  Serial.println(F("Scanning for GY-906 / MLX90614 sensors..."));

  for (uint8_t channel = 0; channel < GY906_CHANNEL_COUNT; channel++) {
    gy906_sensor_on_channel[channel] = false;
    gy906_ambient_c[channel] = 0.0;
    gy906_object_c[channel] = 0.0;
    gy906_last_read_ms[channel] = 0;

    scan_soft_i2c_channel(channel);

    if (select_i2c_hub_channel(channel) && soft_i2c_ping(GY906_ADDR)) {
      gy906_sensor_on_channel[channel] = true;
      Serial.print(F("  GY-906 / MLX90614 found on channel "));
      Serial.println(channel);
    }

    delay(100);
  }

  Serial.println(F("GY-906 scan done."));
}

void setup_gy906() {
  find_gy906_sensors_on_all_channels();
}

void loop_gy906() {
  static unsigned long last_poll = 0;
  static unsigned long last_rescan = 0;

  unsigned long now = millis();
  if (now - last_poll < 1000) {
    return;
  }
  last_poll = now;

  bool anySensor = false;
  static uint8_t next_channel = 0;

  for (uint8_t attempts = 0; attempts < GY906_CHANNEL_COUNT; attempts++) {
    uint8_t channel = next_channel;
    next_channel = (next_channel + 1) % GY906_CHANNEL_COUNT;

    if (!gy906_sensor_on_channel[channel]) continue;

    anySensor = true;

    Serial.print(F("GY-906 channel "));
    Serial.print(channel);
    Serial.print(F(": "));

    if (!select_i2c_hub_channel(channel)) {
      Serial.println(F("failed to select hub channel"));
      break;
    }

    float ambientC;
    float objectC;
    if (read_gy906(ambientC, objectC)) {
      gy906_ambient_c[channel] = ambientC;
      gy906_object_c[channel] = objectC;
      gy906_last_read_ms[channel] = now;

      Serial.print(F("ambient = "));
      Serial.print(ambientC, 2);
      Serial.print(F(" C, object = "));
      Serial.print(objectC, 2);
      Serial.println(F(" C"));
    } else {
      Serial.println(F("read failed"));
    }

    break;
  }

  if (!anySensor && now - last_rescan > 10000) {
    last_rescan = now;
    Serial.println(F("No GY-906 sensors found. Rescanning..."));
    find_gy906_sensors_on_all_channels();
  }
}

bool gy906_get_nth_sensor(uint8_t index, uint8_t &channel, float &ambientC, float &objectC) {
  uint8_t found = 0;

  for (uint8_t ch = 0; ch < GY906_CHANNEL_COUNT; ch++) {
    if (!gy906_sensor_on_channel[ch]) continue;

    if (found == index) {
      channel = ch;
      ambientC = gy906_ambient_c[ch];
      objectC = gy906_object_c[ch];
      return gy906_last_read_ms[ch] != 0;
    }

    found++;
  }

  return false;
}
