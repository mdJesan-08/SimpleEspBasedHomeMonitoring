#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "DhtSensor.h"
#include "GasSensor.h"
#include "Verdict.h"

// WiFi access point + HTTP server that serves the sensor dashboard.
//
// The ESP32 creates its own network, so no router is required. The page is
// stored in flash (see WebPage.h) and polls /api/reading for fresh values.
//
// Routes:
//   GET /             the dashboard page
//   GET /api/reading  the latest reading as JSON
class WebUi {
public:
  WebUi(const char *ssid,
        const char *password,
        uint16_t port = 80,
        uint32_t pollIntervalMs = 2000);

  // Bring up the access point and start listening.
  // Returns false if the AP could not be started, in which case the server is
  // left stopped and handleClient() does nothing.
  bool begin();

  // Service pending HTTP requests. Call this every loop() iteration.
  void handleClient();

  // Publish readings for the next /api/reading request to return.
  // The verdict is recomputed here, so the HTTP handler stays trivial.
  void update(const DhtReading &dht, const GasReading &gas);

  // The AP address to type into a browser, e.g. "192.168.4.1".
  IPAddress ipAddress() const;

  // True once the access point is up.
  bool isRunning() const { return running_; }

private:
  void handleRoot();
  void handleReading();
  void handleNotFound();

  // Serialise `latest_` into a small JSON object. Returns a String rather than
  // taking a buffer because the payload is only ~120 bytes.
  String readingAsJson() const;

  WebServer   server_;
  const char *ssid_;
  const char *password_;
  uint32_t    pollIntervalMs_;
  DhtReading  latestDht_;
  GasReading  latestGas_;
  Verdict     verdict_;
  bool        running_ = false;
};
