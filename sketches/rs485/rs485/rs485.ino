#define RS485_DE_RE 2

#define EVD_ID 1

// Try this first according to manual n2=2:
// 19200 baud, 8 data bits, no parity, 2 stop bits
#define EVD_SERIAL_CONFIG SERIAL_8N2

// If no answer, try this instead:
// #define EVD_SERIAL_CONFIG SERIAL_8O1

uint16_t modbusCRC(const uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;

  for (uint8_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];

    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  return crc;
}

void rs485Transmit() {
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(50);
}

void rs485Receive() {
  Serial3.flush();
  delayMicroseconds(50);
  digitalWrite(RS485_DE_RE, LOW);
}

bool readInputRegisters(uint8_t slave, uint16_t address, uint16_t count, uint16_t *out) {
  uint8_t req[8];

  req[0] = slave;
  req[1] = 0x04; // Read Input Registers
  req[2] = address >> 8;
  req[3] = address & 0xFF;
  req[4] = count >> 8;
  req[5] = count & 0xFF;

  uint16_t crc = modbusCRC(req, 6);
  req[6] = crc & 0xFF;
  req[7] = crc >> 8;

  while (Serial3.available()) Serial3.read();

  rs485Transmit();
  Serial3.write(req, 8);
  rs485Receive();

  uint8_t expectedLen = 5 + 2 * count;
  uint8_t resp[64];

  if (expectedLen > sizeof(resp)) return false;

  unsigned long start = millis();
  uint8_t n = 0;

  while (millis() - start < 300) {
    while (Serial3.available()) {
      if (n < expectedLen) {
        resp[n++] = Serial3.read();
      } else {
        Serial3.read();
      }
    }

    if (n >= expectedLen) break;
  }

  if (n != expectedLen) {
    Serial.print("Timeout / bad length, got bytes: ");
    Serial.println(n);
    return false;
  }

  uint16_t gotCrc = resp[n - 2] | ((uint16_t)resp[n - 1] << 8);
  uint16_t calcCrc = modbusCRC(resp, n - 2);

  if (gotCrc != calcCrc) {
    Serial.println("CRC error");
    return false;
  }

  if (resp[0] != slave || resp[1] != 0x04) {
    Serial.println("Wrong slave/function");
    return false;
  }

  if (resp[2] != 2 * count) {
    Serial.println("Wrong byte count");
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    out[i] = ((uint16_t)resp[3 + 2 * i] << 8) | resp[4 + 2 * i];
  }

  return true;
}

int16_t asSigned(uint16_t x) {
  return (int16_t)x;
}

void setup() {
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);

  Serial.begin(115200);
  Serial3.begin(19200, EVD_SERIAL_CONFIG); // Mega: TX1=18, RX1=19

  delay(1000);
  Serial.println("EVD ice Modbus RTU reader");
}

void loop() {
  uint16_t regs[5];

  bool ok = readInputRegisters(EVD_ID, 0, 5, regs);

  if (!ok) {
    Serial.println("No valid Modbus response");
    Serial.println("Try swapping A/B, checking GND, or changing SERIAL_8N2 to SERIAL_8O1");
    delay(1000);
    return;
  }

  float valvePercent = asSigned(regs[0]) / 10.0;
  float superheatK   = asSigned(regs[1]) / 10.0;
  float suctionC     = asSigned(regs[2]) / 10.0;
  float evapC        = asSigned(regs[3]) / 10.0;
  float evapBar      = asSigned(regs[4]) / 10.0;

  Serial.println("----");
  Serial.print("Valve: "); Serial.print(valvePercent); Serial.println(" %");
  Serial.print("Superheat: "); Serial.print(superheatK); Serial.println(" K");
  Serial.print("Suction temp: "); Serial.print(suctionC); Serial.println(" C");
  Serial.print("Evap temp: "); Serial.print(evapC); Serial.println(" C");
  Serial.print("Evap pressure: "); Serial.print(evapBar); Serial.println(" barg");

  delay(1000);
}