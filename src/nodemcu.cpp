// @iceman — Tarraco Domo | Prototipo pendiente de pruebas
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "config.h"

constexpr uint8_t PIR_PIN = D1;
constexpr uint32_t WARMUP_MS = 60000, COOLDOWN_MS = 30000;
uint32_t lastEvent = 0;
bool previous = false, sent = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("PIR: calentamiento de 60 segundos");
}

void loop() {
  bool online = WiFi.status() == WL_CONNECTED;
  digitalWrite(LED_BUILTIN, online ? LOW : HIGH);
  bool motion = digitalRead(PIR_PIN) == HIGH;
  uint32_t now = millis();
  if (now < WARMUP_MS) { previous = motion; delay(10); return; }
  bool rising = motion && !previous;
  previous = motion;
  if (rising && online && (!sent || uint32_t(now - lastEvent) >= COOLDOWN_MS)) {
    sent = true;
    lastEvent = now;
    for (const char* url : CAMERA_URLS) {
      WiFiClient client;
      HTTPClient http;
      http.setTimeout(2000);
      if (!http.begin(client, url)) { Serial.println("No se pudo iniciar HTTP"); continue; }
      http.addHeader("X-Capture-Token", CAPTURE_TOKEN);
      http.addHeader("Content-Type", "text/plain");
      int code = http.POST("capture");
      Serial.printf("Camara %s: HTTP %d\n", url, code);
      http.end();
      yield();
    }
  }
  delay(10);
}
