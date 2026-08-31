#pragma once

#include <Arduino.h>

#include "DhtSensor.h"
#include "GasSensor.h"

// Overall safety state shown on the dashboard, derived from every sensor.
enum class SafetyState : uint8_t {
  Unknown,  // not enough trustworthy data yet (warming up, or sensor failure)
  Safe,
  Caution,  // worth looking at, not an emergency
  Danger    // act now
};

struct Verdict {
  SafetyState state = SafetyState::Unknown;
  const char *title = "UNKNOWN";
  const char *detail = "Waiting for sensor data";
};

// Comfort bounds for the DHT-22. Outside these it is only a comfort/caution
// signal - never a danger on its own, since a hot room is not an emergency.
#ifndef TEMP_CAUTION_HIGH_C
#define TEMP_CAUTION_HIGH_C 40.0f
#endif
#ifndef TEMP_CAUTION_LOW_C
#define TEMP_CAUTION_LOW_C 5.0f
#endif
#ifndef HUMIDITY_CAUTION_HIGH
#define HUMIDITY_CAUTION_HIGH 80.0f
#endif

// Combine both sensors into a single verdict.
//
// Gas dominates: a tripped methane sensor is unsafe no matter how pleasant the
// temperature is. Temperature and humidity can only raise the state to
// Caution, never to Danger.
//
// If either sensor cannot be trusted the result is Unknown rather than Safe -
// an unread sensor must never be reported as "all clear".
inline Verdict evaluateVerdict(const DhtReading &dht, const GasReading &gas) {
  Verdict v;

  // 1. Gas danger overrides everything else.
  if (gas.valid && gas.level == GasLevel::Danger) {
    v.state  = SafetyState::Danger;
    v.title  = "UNSAFE";
    v.detail = "Gas concentration is high - ventilate and check for a leak";
    return v;
  }

  // 2. A sensor we cannot trust means no verdict, not a safe one.
  if (!gas.valid) {
    v.state = SafetyState::Unknown;
    v.title = "CHECKING";
    v.detail = (gas.level == GasLevel::WarmingUp)
                   ? "Gas sensor is warming up"
                   : "Gas sensor unavailable";
    return v;
  }
  if (!dht.valid) {
    v.state  = SafetyState::Unknown;
    v.title  = "CHECKING";
    v.detail = "Temperature sensor unavailable";
    return v;
  }

  // 3. Environmental comfort checks.
  //
  // There is no gas "caution" tier: the DO output is a single threshold
  // crossing, so the sensor can only say tripped or not. Anything short of a
  // trip is Safe as far as gas is concerned.
  if (dht.temperatureC >= TEMP_CAUTION_HIGH_C) {
    v.state  = SafetyState::Caution;
    v.title  = "CAUTION";
    v.detail = "Temperature is high";
    return v;
  }
  if (dht.temperatureC <= TEMP_CAUTION_LOW_C) {
    v.state  = SafetyState::Caution;
    v.title  = "CAUTION";
    v.detail = "Temperature is low";
    return v;
  }
  if (dht.humidity >= HUMIDITY_CAUTION_HIGH) {
    v.state  = SafetyState::Caution;
    v.title  = "CAUTION";
    v.detail = "Humidity is high";
    return v;
  }

  v.state  = SafetyState::Safe;
  v.title  = "SAFE";
  v.detail = "All readings are within normal range";
  return v;
}

// Machine-readable state name, used as a CSS class by the dashboard.
inline const char *safetyStateName(SafetyState state) {
  switch (state) {
    case SafetyState::Safe:    return "safe";
    case SafetyState::Caution: return "caution";
    case SafetyState::Danger:  return "danger";
    case SafetyState::Unknown:
    default:                   return "unknown";
  }
}
