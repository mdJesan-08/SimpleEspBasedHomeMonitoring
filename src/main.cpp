#include <Arduino.h>

#include <Buzzer.h>
#include <DhtSensor.h>
#include <GasSensor.h>
#include <WebUi.h>

#include "config.h"

DhtSensor dhtSensor(DHT_DATA_PIN, DHT_READ_INTERVAL_MS);
GasSensor gasSensor(MQ4_DIGITAL_PIN, MQ4_DO_ACTIVE_LOW,
                    MQ4_READ_INTERVAL_MS, MQ4_WARMUP_MS, MQ4_CLEAR_SAMPLES);
WebUi     webUi(WIFI_AP_SSID, WIFI_AP_PASSWORD, WEB_SERVER_PORT,
                WEB_POLL_INTERVAL_MS);
Buzzer    buzzer(BUZZER_PIN, BUZZER_ACTIVE_LOW, BUZZER_ON_MS, BUZZER_OFF_MS);

// Decide whether the over-temperature alarm should be sounding.
//
// The two thresholds differ on purpose (see config.h): above ON_C the buzzer
// starts, below OFF_C it stops, and in the band between them whatever it was
// already doing continues. A single threshold would chatter on a reading
// sitting right at the line.
void updateTemperatureAlarm(const DhtReading &reading) {
  if (!reading.valid) {
    // A failed read is not evidence the room got cooler, so an alarm already
    // sounding stays on. Leaving it as-is fails loud rather than silent.
    return;
  }
  const bool wasAlarming = buzzer.alarming();

  if (reading.temperatureC >= BUZZER_TEMP_ON_C) {
    buzzer.setAlarm(true);
  } else if (reading.temperatureC <= BUZZER_TEMP_OFF_C) {
    buzzer.setAlarm(false);
  }

  if (buzzer.alarming() != wasAlarming) {
    Serial.printf("Buzzer %s (%.1f C, on at %.1f / off at %.1f)\n",
                  buzzer.alarming() ? "ON" : "OFF",
                  reading.temperatureC,
                  BUZZER_TEMP_ON_C,
                  BUZZER_TEMP_OFF_C);
  }
}

void setup() {
  // First thing: park the buzzer pin. Until it is driven it floats, and a
  // floating input on an active-low module can sound the buzzer at power-up.
  buzzer.begin();

  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 2000) {
    // give the USB serial a moment to come up
  }

  Serial.println();
  Serial.println(F("Home automation node: DHT-22 + MQ-4"));

  if (BUZZER_SELFTEST_MS > 0) {
    Serial.println(F("Buzzer self-test - you should hear one beep now"));
    buzzer.selfTest(BUZZER_SELFTEST_MS);
  }

  dhtSensor.begin();
  gasSensor.begin();

  Serial.print(F("MQ-4 warming up for "));
  Serial.print(MQ4_WARMUP_MS / 1000UL);
  Serial.println(F(" s - no verdict until then"));

  if (webUi.begin()) {
    Serial.print(F("WiFi access point: "));
    Serial.println(F(WIFI_AP_SSID));
    Serial.print(F("Open http://"));
    Serial.println(webUi.ipAddress());
  } else {
    // The sensors keep logging to Serial even when the AP fails to start.
    Serial.println(F("WiFi access point failed to start"));
  }
}

void loop() {
  webUi.handleClient();

  // The beep pattern runs off millis(), so it must be serviced every pass -
  // not just on the loops where a sensor produced something new.
  buzzer.update();

  // Each sensor rate-limits itself, so poll both every pass and publish
  // whenever either produced something new.
  DhtReading dhtReading;
  GasReading gasReading;

  const bool freshDht = dhtSensor.read(dhtReading);
  const bool freshGas = gasSensor.read(gasReading);

  if (!freshDht && !freshGas) {
    return;
  }

  // Pair the new sample with the other sensor's most recent one, so the
  // verdict always sees a complete picture.
  webUi.update(dhtSensor.last(), gasSensor.last());

  if (freshDht) {
    updateTemperatureAlarm(dhtSensor.last());
    printDhtReading(dhtSensor.last(), Serial);
  }
  if (freshGas) {
    printGasReading(gasSensor.last(), Serial);
  }
}
