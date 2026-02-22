#include "wifi_ap.h"
#include "config.h"

#if TEST_BLINK
void wifiApInit() {}
void wifiApLoop() {}
#else
#include "protocol.h"
#include "control.h"
#include "motor_rear.h"
#include "motor_steer.h"
#include "pins.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiUdp.h>

static WiFiUDP Udp;
static char packetBuffer[256];
static uint32_t lastAckLog = 0;
static bool otaInProgress = false;
static uint8_t otaLastPct = 255;

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

static void setupOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPort(OTA_PORT);
  if (OTA_PASSWORD[0] != '\0') {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    otaLastPct = 255;

    // Safety: stop motion immediately while firmware is being written.
    rearSetSpeed(0);
    steerStop();
    digitalWrite(PIN_RELAY_EN, LOW);

    Serial.println("OTA START");
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    Serial.println("\nOTA END");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total == 0) return;
    const uint8_t pct = (uint8_t)((progress * 100U) / total);
    if (pct != otaLastPct && (pct == 0 || pct == 100 || (pct % 5) == 0)) {
      otaLastPct = pct;
      Serial.printf("OTA %u%%\n", pct);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    Serial.printf("OTA ERROR[%u]\n", (unsigned int)error);
  });

  ArduinoOTA.begin();
  Serial.printf(
    "OTA READY host=%s port=%u ip=%s\n",
    OTA_HOSTNAME,
    (unsigned int)OTA_PORT,
    WiFi.softAPIP().toString().c_str());
}

void wifiApInit() {
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(onWifiEvent);

  const bool apOk = WiFi.softAP(AP_SSID, AP_PASS);
  if (!apOk) {
    Serial.println("AP START FAILED");
  }

  Udp.begin(UDP_PORT);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  setupOta();
}

void wifiApLoop() {
  ArduinoOTA.handle();

  if (otaInProgress) {
    // While OTA is active, ignore runtime packets.
    return;
  }

  const int packetSize = Udp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  const int len = Udp.read(packetBuffer, sizeof(packetBuffer) - 1);
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
#endif
