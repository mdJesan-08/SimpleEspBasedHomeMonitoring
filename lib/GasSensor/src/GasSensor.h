#pragma once

#include <Arduino.h>

// What the MQ-4's comparator output is telling us.
//
// There are only two real states plus the warm-up gate. The module's DO pin is
// a threshold crossing, not a measurement, so there is no "elevated" middle
// ground to report - the trim pot on the board decides where the line is.
enum class GasLevel : uint8_t {
  WarmingUp,  // heater not up to temperature yet - no verdict can be given
  Safe,       // comparator has not tripped
  Danger      // comparator has tripped: gas above the trim-pot threshold
};

// One debounced sample from the MQ-4's digital output.
struct GasReading {
  bool     detected = false;                // comparator currently tripped
  GasLevel level    = GasLevel::WarmingUp;
  bool     valid    = false;                // false while warming up
};

// Non-blocking wrapper around the MQ-4's digital (DO) output.
//
// Why digital and not the AO pin: the ESP32's ADC2 block is handed to the WiFi
// radio when the access point starts, so analogRead on an ADC2 pin returns 0
// once the dashboard is up. Only ADC1 (GPIO 32-39) survives WiFi, and none of
// those pins are broken out on this build. The DO pin sidesteps the ADC
// entirely at the cost of a graded reading.
//
// The MQ-4 is a heated sensor: its output is meaningless until the heater
// reaches temperature, so readings are withheld (level == WarmingUp) for the
// first `warmupMs` after begin(). That is deliberate - a cold sensor reads low,
// which would otherwise display as a confident "SAFE".
class GasSensor {
public:
  // `activeLow` matches the usual LM393 wiring, where DO is pulled LOW when
  // gas exceeds the threshold. Set false for a module that drives it HIGH.
  //
  // `clearSamples` is the number of consecutive quiet samples needed to leave
  // the alarm state. Tripping is immediate; clearing is deliberately slow,
  // because a comparator sitting near its threshold chatters, and an alarm
  // that flickers off is worse than one that lingers.
  GasSensor(uint8_t pin,
            bool activeLow = true,
            unsigned long intervalMs = 2000UL,
            unsigned long warmupMs = 30000UL,
            uint8_t clearSamples = 3);

  // Configure the input pin and start the warm-up clock.
  void begin();

  // Take a sample if the interval has elapsed.
  // Returns true when a fresh sample was taken, false when it is not time yet.
  bool read(GasReading &out);

  // The most recent sample, without triggering a new one.
  const GasReading &last() const { return last_; }

  // Milliseconds of warm-up remaining, 0 once the sensor is ready.
  unsigned long warmupRemainingMs() const;

private:
  // Raw state of the comparator, with the active-low wiring normalised out.
  bool tripped() const;

  uint8_t       pin_;
  bool          activeLow_;
  unsigned long intervalMs_;
  unsigned long warmupMs_;
  uint8_t       clearSamples_;
  unsigned long startedMs_   = 0;
  unsigned long lastReadMs_  = 0;
  uint8_t       clearStreak_ = 0;      // consecutive quiet samples while alarmed
  bool          alarmed_     = false;  // debounced state
  GasReading    last_;
};

// Short label for a level, e.g. "SAFE". Safe to print directly.
const char *gasLevelName(GasLevel level);

// Print a reading as one human-readable line, e.g. to Serial.
void printGasReading(const GasReading &reading, Print &out);
