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
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = 0;

      controlNotifyAppActivity();
      Serial.print("RX ");
      Serial.println(packetBuffer);

      ControlCommand cmd;
      if (protocolParse(packetBuffer, cmd)) {
        controlApply(cmd);
        if (millis() - lastAckLog > 1000) {
          lastAckLog = millis();
          Serial.println("APP OK");
        }
      }
    }

    // Always send status back to sender (even if parse fails)
    char resp[340];
    const int clients = WiFi.softAPgetStationNum();
    const float batt = controlGetBatteryVoltage();
    const char* mode = controlIsManualActive() ? "MANUAL" : "REMOTE";
    const int8_t manualGear = controlGetManualGear();
    const int8_t driveDir = controlGetDriveDir();
    const uint8_t driveSpeed = controlGetDriveSpeedPct();
    const int selFwd = controlGetSelectorFwdActive() ? 1 : 0;
    const int selBack = controlGetSelectorBackActive() ? 1 : 0;
    const float selThrottleV = controlGetSelectorThrottleVoltage();
    const uint8_t selThrottlePct = controlGetSelectorThrottlePct();
    const char* gear = "N";
    if (manualGear > 0) gear = "F";
    else if (manualGear < 0) gear = "R";
    const char* dir = "S";
    if (driveDir > 0) dir = "F";
    else if (driveDir < 0) dir = "R";
    snprintf(
      resp,
      sizeof(resp),
      "{\"ok\":1,\"clients\":%d,\"mode\":\"%s\",\"manual_gear\":\"%s\",\"drive_dir\":\"%s\",\"drive_speed\":%u,\"sel_fwd\":%d,\"sel_back\":%d,\"sel_throttle_v\":%.3f,\"sel_throttle_pct\":%u,\"batt_v\":%.2f,\"ms\":%lu}",
      clients,
      mode,
      gear,
      dir,
      driveSpeed,
      selFwd,
      selBack,
      selThrottleV,
      selThrottlePct,
      batt,
      millis());
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write((const uint8_t*)resp, strlen(resp));
    Udp.endPacket();
  }
}
#endif
