#include <SoftWire.h>

// Soft I2C pins are intentionally reversed compared to the hardware Wire pins.
static const uint8_t SOFT_I2C_SDA_PIN = 20;
static const uint8_t SOFT_I2C_SCL_PIN = 21;

static const uint8_t I2C_HUB_ADDR = 0x70;  // Troyka I2C Hub / PCA9547

SoftWire soft_i2c(SOFT_I2C_SDA_PIN, SOFT_I2C_SCL_PIN);

static uint8_t soft_i2c_rx_buffer[32];
static uint8_t soft_i2c_tx_buffer[32];

void setup_soft_i2c_hub() {
  soft_i2c.setRxBuffer(soft_i2c_rx_buffer, sizeof(soft_i2c_rx_buffer));
  soft_i2c.setTxBuffer(soft_i2c_tx_buffer, sizeof(soft_i2c_tx_buffer));
  soft_i2c.enablePullups(true);
  soft_i2c.setClock(100000);
  soft_i2c.begin();

  if (soft_i2c_ping(I2C_HUB_ADDR)) {
    Serial.println(F("Troyka I2C Hub found at 0x70"));
  } else {
    Serial.println(F("ERROR: Troyka I2C Hub not found at 0x70"));
    Serial.println(F("Check SDA/SCL pins, power, GND, pull-ups."));
  }
}

bool select_i2c_hub_channel(uint8_t channel) {
  if (channel > 7) return false;

  soft_i2c.beginTransmission(I2C_HUB_ADDR);
  soft_i2c.write(channel | 0x08);  // PCA9547: bit 0x08 enables the selected channel
  uint8_t err = soft_i2c.endTransmission();

  delay(5);
  return err == 0;
}

bool soft_i2c_ping(uint8_t addr) {
  soft_i2c.beginTransmission(addr);
  return soft_i2c.endTransmission() == 0;
}

void scan_soft_i2c_bus() {
  int nDevices = 0;

  Serial.println(F("Scanning raw Soft I2C bus..."));

  for (uint8_t addr = 1; addr < 127; addr++) {
    if (soft_i2c_ping(addr)) {
      Serial.print(F("Soft I2C device found at 0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.println(addr, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println(F("No devices found on raw Soft I2C bus"));
  } else {
    Serial.println(F("Raw Soft I2C bus scan done"));
  }
}

void scan_soft_i2c_channel(uint8_t channel) {
  Serial.print(F("Soft I2C channel "));
  Serial.print(channel);
  Serial.println(F(":"));

  if (!select_i2c_hub_channel(channel)) {
    Serial.println(F("  Hub channel select failed"));
    return;
  }

  bool found = false;
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (addr == I2C_HUB_ADDR) continue;

    if (soft_i2c_ping(addr)) {
      found = true;
      Serial.print(F("  I2C device found at 0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.println(addr, HEX);
    }
  }

  if (!found) {
    Serial.println(F("  No devices"));
  }
}

void scan_soft_i2c_hub() {
  Serial.println(F("Scanning Soft I2C hub channels..."));
  for (uint8_t channel = 0; channel < 8; channel++) {
    scan_soft_i2c_channel(channel);
    delay(100);
  }
  Serial.println(F("Soft I2C hub scan done."));
}
