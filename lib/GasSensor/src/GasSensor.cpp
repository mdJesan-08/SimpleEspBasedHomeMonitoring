#include "GasSensor.h"

GasSensor::GasSensor(uint8_t pin,
                     bool activeLow,
                     unsigned long intervalMs,
                     unsigned long warmupMs,
                     uint8_t clearSamples)
    : pin_(pin),
      activeLow_(activeLow),
      intervalMs_(intervalMs),
      warmupMs_(warmupMs),
      clearSamples_(clearSamples == 0 ? 1 : clearSamples) {}

void GasSensor::begin() {
  // The LM393 comparator on these modules is open-collector. Most boards fit
  // their own pull-up, but enabling the internal one costs nothing and keeps
  // the input from floating on a board that does not.
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);

  startedMs_ = millis();
  // Allow the first sample immediately; the warm-up gate withholds the verdict.
  lastReadMs_ = startedMs_ - intervalMs_;
}

unsigned long GasSensor::warmupRemainingMs() const {
  const unsigned long elapsed = millis() - startedMs_;
  if (elapsed >= warmupMs_) {
    return 0;
  }
  return warmupMs_ - elapsed;
}

bool GasSensor::tripped() const {
  const int raw = digitalRead(pin_);
  return activeLow_ ? (raw == LOW) : (raw == HIGH);
}

bool GasSensor::read(GasReading &out) {
  if (millis() - lastReadMs_ < intervalMs_) {
    return false;
  }
  lastReadMs_ = millis();

  GasReading reading;

  if (warmupRemainingMs() > 0) {
    // A cold MQ-4 reads high on power-up and settles downward, so the
    // comparator is often tripped during warm-up. Hold the debounce state
    // clear rather than letting that latch into a false alarm the moment the
    // warm-up gate lifts.
    alarmed_     = false;
    clearStreak_ = 0;

    reading.detected = false;
    reading.level    = GasLevel::WarmingUp;
    reading.valid    = false;
  } else {
    // Asymmetric debounce: trip on the first hit, but only stand down after
    // `clearSamples_` consecutive quiet reads.
    if (tripped()) {
      alarmed_     = true;
      clearStreak_ = 0;
    } else if (alarmed_) {
      if (++clearStreak_ >= clearSamples_) {
        alarmed_     = false;
        clearStreak_ = 0;
      }
    }

    reading.detected = alarmed_;
    reading.level    = alarmed_ ? GasLevel::Danger : GasLevel::Safe;
    reading.valid    = true;
  }

  last_ = reading;
  out   = reading;
  return true;
}

const char *gasLevelName(GasLevel level) {
  switch (level) {
    case GasLevel::Safe:   return "SAFE";
    case GasLevel::Danger: return "GAS DETECTED";
    case GasLevel::WarmingUp:
    default:               return "WARMING UP";
  }
}

void printGasReading(const GasReading &reading, Print &out) {
  out.printf("Gas (MQ-4): %s\n", gasLevelName(reading.level));
}
