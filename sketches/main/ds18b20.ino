#include <GyverDS18.h>

#define DS18B20_PIN A10
#define DS18B20_RESOLUTION 12
#define DS18B20_HISTORY_SIZE 32

GyverDS18Single ds18b20(DS18B20_PIN);

double ds18b20_history[DS18B20_HISTORY_SIZE];
uint8_t ds18b20_history_size = 0;
uint8_t ds18b20_history_pos = 0;
bool ds18b20_has_value = false;
double ds18b20_last_temp_c = 0.0;

bool ds18b20_is_valid_temp(double tempC) {
  return tempC > -100.0 && tempC < 150.0;
}

void ds18b20_push_temp(double tempC) {
  ds18b20_history[ds18b20_history_pos] = tempC;
  ds18b20_history_pos = (ds18b20_history_pos + 1) % DS18B20_HISTORY_SIZE;

  if (ds18b20_history_size < DS18B20_HISTORY_SIZE) {
    ds18b20_history_size++;
  }

  ds18b20_last_temp_c = tempC;
  ds18b20_has_value = true;
}

void setup_ds18b20() {
  ds18b20.setResolution(DS18B20_RESOLUTION);
}

void loop_ds18b20() {
  if (ds18b20.tick() == 0) {
    double tempC = ds18b20.getTemp();

    if (ds18b20_is_valid_temp(tempC)) {
      ds18b20_push_temp(tempC);
    } else {
      Serial.print(F("DS18B20 invalid temperature: "));
      Serial.println(tempC);
    }
  }
}

bool ds18b20_get_latest(double &tempC) {
  if (!ds18b20_has_value) return false;

  tempC = ds18b20_last_temp_c;
  return true;
}

bool ds18b20_get_average(double &tempC) {
  if (ds18b20_history_size == 0) return false;

  double sum = 0.0;
  for (uint8_t i = 0; i < ds18b20_history_size; i++) {
    sum += ds18b20_history[i];
  }

  tempC = sum / ds18b20_history_size;
  return true;
}

void fetch_temp() {
  double latest;
  double average;

  if (!ds18b20_get_latest(latest)) {
    Serial.println(F("DS18B20: no data yet"));
    return;
  }

  Serial.print(F("DS18B20 latest: "));
  Serial.println(latest);

  if (ds18b20_get_average(average)) {
    Serial.print(F("DS18B20 average: "));
    Serial.println(average);
  }
}
