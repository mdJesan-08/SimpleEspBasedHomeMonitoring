#include "WebUi.h"

#include <WiFi.h>

#include "WebPage.h"

namespace {

// JSON has no NaN literal, so an invalid reading is sent as null. Emitting a
// bare `nan` here would make JSON.parse() throw in the browser.
void appendNumber(String &json, const char *key, float value, bool valid) {
  json += '"';
  json += key;
  json += "\":";
  if (valid && !isnan(value)) {
    json += String(value, 1);
  } else {
    json += "null";
  }
}

}  // namespace

WebUi::WebUi(const char *ssid,
             const char *password,
             uint16_t port,
             uint32_t pollIntervalMs)
    : server_(port),
      ssid_(ssid),
      password_(password),
      pollIntervalMs_(pollIntervalMs) {}

bool WebUi::begin() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ssid_, password_)) {
    return false;
  }

  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/reading", HTTP_GET, [this]() { handleReading(); });
  server_.onNotFound([this]() { handleNotFound(); });

  server_.begin();
  running_ = true;
  return true;
}

void WebUi::handleClient() {
  if (running_) {
    server_.handleClient();
  }
}

void WebUi::update(const DhtReading &dht, const GasReading &gas) {
  latestDht_ = dht;
  latestGas_ = gas;
  verdict_   = evaluateVerdict(dht, gas);
}

IPAddress WebUi::ipAddress() const {
  return WiFi.softAPIP();
}

void WebUi::handleRoot() {
  // FPSTR pulls the page out of flash without copying it first.
  String page = FPSTR(WEB_PAGE_INDEX);

  // Substitute the poll interval so the value lives in config.h only.
  page.replace("%%POLL_MS%%", String(pollIntervalMs_));

  server_.send(200, "text/html; charset=utf-8", page);
}

void WebUi::handleReading() {
  // The browser polls this endpoint, so it must never be cached.
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", readingAsJson());
}

void WebUi::handleNotFound() {
  server_.send(404, "text/plain", "Not found");
}

String WebUi::readingAsJson() const {
  const bool dhtValid = latestDht_.valid;

  String json;
  json.reserve(320);

  json += '{';
  appendNumber(json, "temperatureC", latestDht_.temperatureC, dhtValid);
  json += ',';
  appendNumber(json, "temperatureF", latestDht_.temperatureF, dhtValid);
  json += ',';
  appendNumber(json, "humidity", latestDht_.humidity, dhtValid);
  json += ',';
  appendNumber(json, "heatIndexC", latestDht_.heatIndexC, dhtValid);
  json += ",\"valid\":";
  json += dhtValid ? "true" : "false";

  json += ",\"gasDetected\":";
  json += latestGas_.detected ? "true" : "false";
  json += ",\"gasLevel\":\"";
  json += gasLevelName(latestGas_.level);
  json += "\",\"gasValid\":";
  json += latestGas_.valid ? "true" : "false";

  json += ",\"verdict\":{\"state\":\"";
  json += safetyStateName(verdict_.state);
  json += "\",\"title\":\"";
  json += verdict_.title;
  json += "\",\"detail\":\"";
  json += verdict_.detail;
  json += "\"}}";

  return json;
}
