#pragma once

#include <Arduino.h>
#include <DHT.h>

// One complete sample from the DHT-22.
// `valid` is false when the sensor did not answer or returned NaN, in which
// case the other fields must be ignored.
struct DhtReading {
  float temperatureC = NAN;
  float temperatureF = NAN;
  float humidity     = NAN;  // relative humidity, %
  float heatIndexC   = NAN;  // perceived temperature
  bool  valid        = false;
};

// Thin, non-blocking wrapper around the Adafruit DHT driver.
// It rate-limits the sensor for you, so loop() never has to call delay().
class DhtSensor {
public:
  DhtSensor(uint8_t pin,
            unsigned long intervalMs = 2000UL,
            uint8_t type = DHT22);

  // Start the sensor. The first sample becomes available one interval later,
  // which gives the DHT-22 time to settle after power-up.
  void begin();

  // Take a sample if the interval has elapsed.
  // Returns true when a fresh attempt was made (check `out.valid` to see
  // whether it succeeded), false when it is simply not time yet.
  bool read(DhtReading &out);

  // The most recent attempt, without triggering a new one.
  const DhtReading &last() const { return last_; }

private:
  DHT           dht_;
  unsigned long intervalMs_;
  unsigned long lastReadMs_ = 0;
  DhtReading    last_;
};

// Print a reading as one human-readable line, e.g. to Serial.
void printDhtReading(const DhtReading &reading, Print &out);
