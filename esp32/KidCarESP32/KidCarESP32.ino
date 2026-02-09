#include "config.h"
#include "pins.h"
#include "wifi_ap.h"
#include "protocol.h"
#include "control.h"

#if TEST_BLINK
// RGB LED pin (common for ESP32-S3 boards). If no light, try 38.
#ifndef RGB_PIN
#define RGB_PIN 48
#endif
#endif

void setup() {
  Serial.begin(115200);
  delay(200);

#if TEST_BLINK
  // On many ESP32-S3 boards the RGB LED is on GPIO48 (some revisions use GPIO38).
  // If blue doesn't blink on GPIO48, change to 38.
#else
  // Force RGB off on common pins
  rgbLedWrite(48, 0, 0, 0);
  rgbLedWrite(38, 0, 0, 0);
  rgbLedWrite(2, 0, 0, 0);
  rgbLedWrite(21, 0, 0, 0);

  setupPins();
  setupPwm();
  controlInit();

  wifiApInit();
#endif
}

void loop() {
#if TEST_BLINK
  // RGB ring counter: rotate through colors
  static uint8_t step = 0;
  const uint8_t colors[][3] = {
    {64, 0, 0},   // red
    {64, 32, 0},  // orange
    {64, 64, 0},  // yellow
    {0, 64, 0},   // green
    {0, 64, 64},  // cyan
    {0, 0, 64},   // blue
    {32, 0, 64},  // purple
  };
  const uint8_t count = sizeof(colors) / sizeof(colors[0]);
  rgbLedWrite(RGB_PIN, colors[step][0], colors[step][1], colors[step][2]);
  Serial.print("RGB step=");
  Serial.print(step);
  Serial.print(" R=");
  Serial.print(colors[step][0]);
  Serial.print(" G=");
  Serial.print(colors[step][1]);
  Serial.print(" B=");
  Serial.println(colors[step][2]);
  step = (step + 1) % count;
  delay(400);
#else
  static uint32_t lastLog = 0;
  if (millis() - lastLog >= 5000) {
    lastLog = millis();
    Serial.println("KIDCAR RUN");
  }
  wifiApLoop();
  controlLoop();
#endif
}
