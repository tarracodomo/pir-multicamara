#pragma once
// @iceman — Tarraco Domo. Copiar como config.h y sustituir ejemplos.
// Son ejemplos públicos: nunca utilizarlos como credenciales reales.
#define WIFI_SSID "REEMPLAZAR_RED_24_GHZ"
#define WIFI_PASSWORD "REEMPLAZAR_PASSWORD_WIFI"
#define CAPTURE_TOKEN "REEMPLAZAR_POR_TOKEN_ALEATORIO_DE_AL_MENOS_32_CARACTERES"
// Reservar estas IP en el DHCP del router; aquí solo se configuran destinos.
static const char* CAMERA_URLS[] = {
  "http://192.168.1.100/capture",
  "http://192.168.1.101/capture",
  "http://192.168.1.102/capture"
};
