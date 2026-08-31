#include "DhtSensor.h"

DhtSensor::DhtSensor(uint8_t pin, unsigned long intervalMs, uint8_t type)
    : dht_(pin, type), intervalMs_(intervalMs) {}

void DhtSensor::begin() {
  dht_.begin();
  // Hold off the first sample by one full interval so the sensor can settle.
  lastReadMs_ = millis();
}

bool DhtSensor::read(DhtReading &out) {
  if (millis() - lastReadMs_ < intervalMs_) {
    return false;
  }
  lastReadMs_ = millis();

  DhtReading reading;
  reading.humidity     = dht_.readHumidity();
  reading.temperatureC = dht_.readTemperature();
  reading.valid        = !isnan(reading.humidity) && !isnan(reading.temperatureC);

  if (reading.valid) {
    reading.temperatureF = dht_.convertCtoF(reading.temperatureC);
    reading.heatIndexC   = dht_.computeHeatIndex(reading.temperatureC,
                                                 reading.humidity,
                                                 false /* Celsius */);
  }

  last_ = reading;
  out   = reading;
  return true;
}

void printDhtReading(const DhtReading &reading, Print &out) {
  if (!reading.valid) {
    out.println(F("DHT-22: read failed - check wiring and pull-up resistor"));
    return;
  }

  out.printf("Temperature: %.1f C (%.1f F)  Humidity: %.1f %%  Heat index: %.1f C\n",
             reading.temperatureC,
             reading.temperatureF,
             reading.humidity,
             reading.heatIndexC);
}
