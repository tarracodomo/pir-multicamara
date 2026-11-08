// @iceman — Tarraco Domo | Solo AI-Thinker ESP32-CAM con PSRAM
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <esp_system.h>
#include <FS.h>
#include <SD_MMC.h>
#include "config.h"

WebServer server(80);
bool ready = false, busy = false, pending = false, hasEvent = false;
uint32_t lastEvent = 0, sessionId = 0, sequence = 0;
const char* headerNames[] = {"X-Capture-Token"};

bool initCamera() {
  if (!psramFound()) return false;
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = 5; c.pin_d1 = 18; c.pin_d2 = 19; c.pin_d3 = 21;
  c.pin_d4 = 36; c.pin_d5 = 39; c.pin_d6 = 34; c.pin_d7 = 35;
  c.pin_xclk = 0; c.pin_pclk = 22; c.pin_vsync = 25; c.pin_href = 23;
  c.pin_sccb_sda = 26; c.pin_sccb_scl = 27;
  c.pin_pwdn = 32; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000; c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_VGA; c.jpeg_quality = 12; c.fb_count = 2;
  c.fb_location = CAMERA_FB_IN_PSRAM; c.grab_mode = CAMERA_GRAB_LATEST;
  return esp_camera_init(&c) == ESP_OK;
}

bool savePhoto(unsigned index) {
  // Descarta frames previos. La frescura real debe comprobarse en hardware.
  for (int i = 0; i < 2; ++i) {
    camera_fb_t* old = esp_camera_fb_get();
    if (!old) return false;
    esp_camera_fb_return(old);
    delay(100);
  }
  camera_fb_t* frame = esp_camera_fb_get();
  if (!frame) return false;
  char path[80];
  snprintf(path, sizeof(path), "/capturas/%08lx_%lu_%u.jpg",
    (unsigned long)sessionId, (unsigned long)sequence, index);
  if (SD_MMC.exists(path)) { esp_camera_fb_return(frame); return false; }
  File file = SD_MMC.open(path, FILE_WRITE);
  bool ok = false;
  if (file) {
    ok = file.write(frame->buf, frame->len) == frame->len;
    file.close();
  }
  esp_camera_fb_return(frame);
  if (!ok) SD_MMC.remove(path);
  Serial.printf("Foto %s: %s\n", path, ok ? "OK" : "ERROR");
  return ok;
}

void captureRequest() {
  if (server.header("X-Capture-Token") != CAPTURE_TOKEN) {
    server.send(401, "text/plain", "No autorizado"); return;
  }
  if (!ready) { server.send(503, "text/plain", "Camara o SD no disponible"); return; }
  if (busy || pending || (hasEvent && uint32_t(millis() - lastEvent) < 30000)) {
    server.send(429, "text/plain", "Espera entre secuencias"); return;
  }
  hasEvent = true; lastEvent = millis(); pending = true;
  server.send(202, "text/plain", "Orden aceptada; comprobar guardado por serial o SD");
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT); digitalWrite(4, LOW);
  bool cameraOK = initCamera();
  bool sdOK = SD_MMC.begin("/sdcard", true);
  if (sdOK && SD_MMC.cardType() != CARD_NONE) {
    if (!SD_MMC.exists("/capturas")) sdOK = SD_MMC.mkdir("/capturas");
  } else sdOK = false;
  ready = cameraOK && sdOK;
  sessionId = esp_random();
  Serial.printf("Camara: %s | SD: %s\n", cameraOK ? "OK" : "ERROR", sdOK ? "OK" : "ERROR");
  WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  server.collectHeaders(headerNames, 1);
  server.on("/capture", HTTP_POST, captureRequest);
  server.onNotFound([]() { server.send(404, "text/plain", "No encontrado"); });
  server.begin();
}

void loop() {
  static bool wasOnline = false;
  bool online = WiFi.status() == WL_CONNECTED;
  if (online && !wasOnline) Serial.println(WiFi.localIP());
  wasOnline = online;
  server.handleClient();
  if (pending) {
    pending = false; busy = true; ++sequence;
    delay(100);
    unsigned saved = 0;
    for (unsigned i = 1; i <= 3; ++i) {
      if (savePhoto(i)) ++saved;
      delay(1000);
    }
    Serial.printf("Secuencia %lu: %u/3 fotos guardadas\n", (unsigned long)sequence, saved);
    busy = false;
  }
  delay(2);
}
