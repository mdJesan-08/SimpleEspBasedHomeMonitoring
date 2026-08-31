#pragma once

// Hardware / behaviour configuration for the home-automation node.
// Keep every board-specific number here so the libraries stay reusable.

// Serial console
#define SERIAL_BAUD 115200

// DHT-22 wiring (ESP32 / NodeMCU-32S)
//   VCC  -> 3V3
//   DATA -> GPIO 4  (10k pull-up resistor between DATA and 3V3)
//   GND  -> GND
#define DHT_DATA_PIN 4

// The DHT-22 needs ~2 s between readings, otherwise it returns a stale
// or failed value.
#define DHT_READ_INTERVAL_MS 2000UL

// ---------------------------------------------------------------------------
// MQ-4 methane / natural-gas sensor  (digital DO output)
// ---------------------------------------------------------------------------
//   VCC -> 5V   (the heater needs 5 V)
//   GND -> GND
//   DO  -> GPIO 18, THROUGH A DIVIDER - see below
//
// IMPORTANT: most MQ breakouts fit their own pull-up from DO to VCC, so with
// the module on 5 V the idle-HIGH level is 5 V. ESP32 pins are NOT 5 V
// tolerant. Drop it with a divider - 10k from DO to the GPIO, 20k from the
// GPIO to GND, giving ~3.3 V - or confirm with a meter that your board's DO
// idles at 3.3 V before wiring it straight in.
//
// This build reads the module's DIGITAL output, not AO. The ESP32 hands the
// ADC2 block to the WiFi radio, so analogRead on GPIO 2/4/15 returns 0 once
// the access point starts - and ADC1 (GPIO 32-39), the only WiFi-safe analog
// block, is not broken out on this board. DO avoids the ADC entirely.
//
// The trade-off: DO is a threshold crossing, so there is no graded reading and
// no raw number. Sensitivity is set by the trim pot on the module itself.
#define MQ4_DIGITAL_PIN 18

// Most MQ modules use an LM393 comparator that pulls DO LOW when gas exceeds
// the trim-pot threshold. Set to 0 if yours drives DO HIGH instead - check by
// watching the module's own indicator LED against the Serial output.
#define MQ4_DO_ACTIVE_LOW 1

// How often to poll the DO pin. Matching the web poll rate keeps the dashboard
// consistent; the MQ-4's own response time is seconds, so faster buys nothing.
#define MQ4_READ_INTERVAL_MS 2000UL

// The heater needs to reach temperature before readings mean anything. Until
// this has elapsed the sensor reports a "warming up" state instead of a
// verdict, so a cold sensor never shows a falsely reassuring "safe".
// The datasheet asks for 24 h+ of burn-in for a brand-new sensor; 30 s is the
// per-power-up warm-up.
#define MQ4_WARMUP_MS 30000UL

// Consecutive quiet samples required to leave the alarm state. Tripping is
// immediate; clearing waits this many reads, because a comparator sitting near
// its threshold chatters and a flickering gas alarm is worse than a slow one.
// At the interval above, 3 samples is about 6 s.
#define MQ4_CLEAR_SAMPLES 3

// --- Calibrating the trip point --------------------------------------------
// THIS MUST BE DONE ON THE HARDWARE - there is no threshold in software now.
//
// Power the board and let the MQ-4 warm up. In clean air, turn the module's
// trim pot until its DO indicator LED just switches off (Serial shows SAFE).
// Back it off slightly, then confirm with a real gas source - a brief puff
// from an UNLIT lighter - that it trips to GAS DETECTED and recovers.

// ---------------------------------------------------------------------------
// Buzzer  (active-low module)
// ---------------------------------------------------------------------------
//   VCC -> 5V   (or 3V3 - check the module; many work on either)
//   GND -> GND
//   I/O -> GPIO 19
//
// GPIO 19 is not a strapping pin, which matters here: a buzzer line held at
// the wrong level while the ESP32 boots can stop it starting. Avoid 2, 5 and
// 15 for this.
#define BUZZER_PIN 19

// Pulling the signal pin LOW sounds this buzzer. Set to 0 for active-high.
#define BUZZER_ACTIVE_LOW 1

// Beep pattern while the alarm is on: sound for ON_MS, silence for OFF_MS.
// Intermittent beeping carries further than a continuous tone.
#define BUZZER_ON_MS  200UL
#define BUZZER_OFF_MS 800UL

// Beep once at boot to prove the buzzer is wired correctly, before any sensor
// is involved. Set to 0 once you are happy with the wiring.
#define BUZZER_SELFTEST_MS 300UL

// --- Over-temperature alarm ------------------------------------------------
// Start beeping at or above this temperature.
#define BUZZER_TEMP_ON_C 31.5f

// Stop again at or below this one. The half-degree gap is hysteresis: without
// it, a reading hovering at exactly 31.5 would switch the buzzer on and off
// every couple of seconds.
#define BUZZER_TEMP_OFF_C 31.0f

// ---------------------------------------------------------------------------
// WiFi access point
// ---------------------------------------------------------------------------
// The ESP32 creates its own network; no router is involved. Connect a phone or
// laptop to this SSID, then open http://192.168.4.1 in a browser.
#define WIFI_AP_SSID     "homeAutomation"
#define WIFI_AP_PASSWORD "22321041"   // WPA2 requires at least 8 characters

// HTTP port for the built-in web server.
#define WEB_SERVER_PORT 80

// How often the browser polls the ESP32 for a fresh reading, in milliseconds.
// Keep this at or above DHT_READ_INTERVAL_MS - polling faster only returns the
// same cached sample.
#define WEB_POLL_INTERVAL_MS 2000
