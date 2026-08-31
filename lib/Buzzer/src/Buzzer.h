#pragma once

#include <Arduino.h>

// Non-blocking driver for a simple on/off buzzer module.
//
// The buzzer beeps intermittently (on, off, on, ...) for as long as the alarm
// is asserted, which carries much further than a continuous tone and is far
// easier to ignore-proof. Nothing here blocks: call update() every loop and
// the pattern advances off millis().
//
// This drives a module with its own driver transistor - the kind with VCC/GND
// and a signal pin. It is not a passive piezo, so there is no tone() here.
class Buzzer {
public:
  // `activeLow` matches the common module where pulling the signal pin LOW
  // sounds the buzzer.
  Buzzer(uint8_t pin,
         bool activeLow = true,
         unsigned long onMs = 200UL,
         unsigned long offMs = 800UL);

  // Park the pin in the silent state. Call this early in setup(): until a pin
  // is driven it floats, and a floating input on an active-low module can let
  // the buzzer sound on power-up.
  void begin();

  // Assert or release the alarm. Asserting starts a beep immediately;
  // releasing silences the buzzer on the spot rather than finishing the beep.
  // Calling it repeatedly with the same value is free - the pattern keeps its
  // place rather than restarting.
  void setAlarm(bool on);

  // Advance the beep pattern. Call every loop().
  void update();

  // Sound the buzzer for `ms`, then silence it. This one DOES block, which is
  // fine in setup(): it exists to prove the wiring works before any sensor
  // reading is involved. If you hear nothing here, the problem is the module,
  // the pin, or the active-low setting - not the alarm logic.
  void selfTest(unsigned long ms = 300UL);

  // True while the alarm is asserted (not whether the tone is currently on).
  bool alarming() const { return alarming_; }

private:
  // Drive the pin, translating for active-low wiring.
  void write(bool sounding);

  uint8_t       pin_;
  bool          activeLow_;
  unsigned long onMs_;
  unsigned long offMs_;
  bool          alarming_    = false;  // alarm asserted by the caller
  bool          sounding_    = false;  // buzzer currently making noise
  unsigned long phaseFromMs_ = 0;      // when the current on/off phase began
};
