#include "wifi_ap.h"
#include "config.h"

#if TEST_BLINK
void wifiApInit() {}
void wifiApLoop() {}
#else
#include "protocol.h"
#include "control.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>

static WiFiUDP Udp;
static char packetBuffer[256];
static uint32_t lastAckLog = 0;
static bool faultState = false;

static void onWifiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("AP START");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("AP STOP");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("STA CONNECTED");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("STA DISCONNECTED");
      break;
    default:
      break;
  }
}

void wifiApInit() {
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(onWifiEvent);
  WiFi.softAP(AP_SSID, AP_PASS);
  Udp.begin(UDP_PORT);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void wifiApLoop() {
  const uint32_t now = millis();

  int packetSize = Udp.parsePacket();
  if (packetSize <= 0) return;

  int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
  if (len <= 0) return;
  packetBuffer[len] = 0;

  controlNotifyAppActivity();
  Serial.print("RX ");
  Serial.println(packetBuffer);

  // Always send status back to sender (even if parse fails)
  char resp[160];
  int clients = WiFi.softAPgetStationNum();
  const char* status = faultState ? "FAULT" : "OK";
  snprintf(resp, sizeof(resp), "{\"ok\":1,\"status\":\"%s\",\"clients\":%d,\"ms\":%lu}", status, clients, millis());
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write((const uint8_t*)resp, strlen(resp));
  Udp.endPacket();

  ControlCommand cmd;
  if (protocolParse(packetBuffer, cmd)) {
    controlApply(cmd);
    if (millis() - lastAckLog > 1000) {
      lastAckLog = millis();
      Serial.println("APP OK");
    }
  }
}
#endif
