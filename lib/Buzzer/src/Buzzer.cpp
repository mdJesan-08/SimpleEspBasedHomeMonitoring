#include "Buzzer.h"

Buzzer::Buzzer(uint8_t pin,
               bool activeLow,
               unsigned long onMs,
               unsigned long offMs)
    : pin_(pin),
      activeLow_(activeLow),
      onMs_(onMs),
      offMs_(offMs) {}

void Buzzer::write(bool sounding) {
  digitalWrite(pin_, activeLow_ ? (sounding ? LOW : HIGH)
                                : (sounding ? HIGH : LOW));
}

void Buzzer::begin() {
  // Set the level before switching to OUTPUT, so the pin never drives the
  // sounding state for even an instant on the way up.
  write(false);
  pinMode(pin_, OUTPUT);
  write(false);

  sounding_    = false;
  phaseFromMs_ = millis();
}

void Buzzer::setAlarm(bool on) {
  if (on == alarming_) {
    return;  // no change - let the current pattern run on undisturbed
  }
  alarming_    = on;
  sounding_    = on;  // lead with a beep so the alarm is audible at once
  phaseFromMs_ = millis();
  write(sounding_);
}

void Buzzer::update() {
  if (!alarming_) {
    return;
  }

  const unsigned long phaseMs = sounding_ ? onMs_ : offMs_;
  if (millis() - phaseFromMs_ < phaseMs) {
    return;
  }

  sounding_    = !sounding_;
  phaseFromMs_ = millis();
  write(sounding_);
}

void Buzzer::selfTest(unsigned long ms) {
  write(true);
  delay(ms);
  write(false);
  sounding_    = false;
  phaseFromMs_ = millis();
}
