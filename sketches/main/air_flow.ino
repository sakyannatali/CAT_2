// -------------------- Air flow calibration --------------------
// RPS -> passed air volume conversion table.
// Edit these values after calibration. Conversion uses linear interpolation.
#define AIR_FLOW_CALIBRATION_TABLE_SIZE 2

const double AIR_FLOW_CALIBRATION_RPS[AIR_FLOW_CALIBRATION_TABLE_SIZE] = {
  0.0, 100000.0
};

const double AIR_FLOW_CALIBRATION_VOLUME[AIR_FLOW_CALIBRATION_TABLE_SIZE] = {
  0.0, 100000.0
};

// -------------------- Air flow sensor config --------------------
#define AIR_FLOW_SENSOR_COUNT 2
#define AIR_FLOW_SAMPLE_INTERVAL_MS 5
#define AIR_FLOW_UPDATE_INTERVAL_MS 1000
#define AIR_FLOW_THRESHOLD_HIGH 700
#define AIR_FLOW_THRESHOLD_LOW 300
#define AIR_FLOW_PULSES_PER_REV 1.0

struct AirFlowSensor {
  uint8_t pin;
  bool high_state;
  unsigned long pulse_count;
  double rps;
  double volume;
  int last_raw;
  bool has_value;
};

AirFlowSensor air_flow_sensors[AIR_FLOW_SENSOR_COUNT] = {
  {A0, false, 0, 0.0, 0.0, 0, false},
  {A1, false, 0, 0.0, 0.0, 0, false}
};

double air_flow_rps_to_volume(double rps) {
  if (AIR_FLOW_CALIBRATION_TABLE_SIZE <= 0) return 0.0;

  if (rps <= AIR_FLOW_CALIBRATION_RPS[0]) {
    return AIR_FLOW_CALIBRATION_VOLUME[0];
  }

  for (uint8_t i = 1; i < AIR_FLOW_CALIBRATION_TABLE_SIZE; i++) {
    double rpsPrev = AIR_FLOW_CALIBRATION_RPS[i - 1];
    double rpsNext = AIR_FLOW_CALIBRATION_RPS[i];

    if (rps <= rpsNext) {
      double volumePrev = AIR_FLOW_CALIBRATION_VOLUME[i - 1];
      double volumeNext = AIR_FLOW_CALIBRATION_VOLUME[i];

      if (rpsNext == rpsPrev) return volumeNext;

      double k = (rps - rpsPrev) / (rpsNext - rpsPrev);
      return volumePrev + (volumeNext - volumePrev) * k;
    }
  }

  return AIR_FLOW_CALIBRATION_VOLUME[AIR_FLOW_CALIBRATION_TABLE_SIZE - 1];
}

void setup_air_flow() {
  for (uint8_t i = 0; i < AIR_FLOW_SENSOR_COUNT; i++) {
    air_flow_sensors[i].high_state = false;
    air_flow_sensors[i].pulse_count = 0;
    air_flow_sensors[i].rps = 0.0;
    air_flow_sensors[i].volume = 0.0;
    air_flow_sensors[i].last_raw = 0;
    air_flow_sensors[i].has_value = false;
  }
}

void air_flow_sample_sensor(uint8_t index) {
  AirFlowSensor &sensor = air_flow_sensors[index];
  int raw = analogRead(sensor.pin);
  sensor.last_raw = raw;

  if (!sensor.high_state && raw > AIR_FLOW_THRESHOLD_HIGH) {
    sensor.high_state = true;
    sensor.pulse_count++;
  } else if (sensor.high_state && raw < AIR_FLOW_THRESHOLD_LOW) {
    sensor.high_state = false;
  }
}

void loop_air_flow() {
  static unsigned long last_sample_ms = 0;
  static unsigned long last_update_ms = 0;

  unsigned long now = millis();

  if (now - last_sample_ms >= AIR_FLOW_SAMPLE_INTERVAL_MS) {
    last_sample_ms = now;

    for (uint8_t i = 0; i < AIR_FLOW_SENSOR_COUNT; i++) {
      air_flow_sample_sensor(i);
    }
  }

  unsigned long updateDelta = now - last_update_ms;
  if (updateDelta >= AIR_FLOW_UPDATE_INTERVAL_MS) {
    last_update_ms = now;

    double windowSeconds = updateDelta / 1000.0;
    if (windowSeconds <= 0.0) windowSeconds = 1.0;

    for (uint8_t i = 0; i < AIR_FLOW_SENSOR_COUNT; i++) {
      AirFlowSensor &sensor = air_flow_sensors[i];

      double revolutions = sensor.pulse_count / AIR_FLOW_PULSES_PER_REV;
      sensor.rps = revolutions / windowSeconds;
      sensor.volume = air_flow_rps_to_volume(sensor.rps);
      sensor.pulse_count = 0;
      sensor.has_value = true;

      Serial.print(F("Air flow sensor "));
      Serial.print(i + 1);
      Serial.print(F(": raw = "));
      Serial.print(sensor.last_raw);
      Serial.print(F(", rps = "));
      Serial.print(sensor.rps, 2);
      Serial.print(F(", volume = "));
      Serial.println(sensor.volume, 2);
    }
  }
}

bool air_flow_get_rps(uint8_t index, double &rps) {
  if (index >= AIR_FLOW_SENSOR_COUNT) return false;
  if (!air_flow_sensors[index].has_value) return false;

  rps = air_flow_sensors[index].rps;
  return true;
}

bool air_flow_get_volume(uint8_t index, double &volume) {
  if (index >= AIR_FLOW_SENSOR_COUNT) return false;
  if (!air_flow_sensors[index].has_value) return false;

  volume = air_flow_sensors[index].volume;
  return true;
}

void fetch_air_flow() {
  for (uint8_t i = 0; i < AIR_FLOW_SENSOR_COUNT; i++) {
    AirFlowSensor &sensor = air_flow_sensors[i];

    Serial.print(F("Air flow sensor "));
    Serial.print(i + 1);
    Serial.print(F(": "));

    if (!sensor.has_value) {
      Serial.println(F("no data yet"));
      continue;
    }

    Serial.print(F("raw = "));
    Serial.print(sensor.last_raw);
    Serial.print(F(", rps = "));
    Serial.print(sensor.rps, 2);
    Serial.print(F(", volume = "));
    Serial.println(sensor.volume, 2);
  }
}
