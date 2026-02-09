#include "control.h"
#include "motor_rear.h"
#include "motor_steer.h"
#include "config.h"
#include <Arduino.h>

static ControlCommand lastCmd = {0, 0, 0, 0};
static uint8_t colorIndex = 0;
static int lastStepInput = 0;
static uint32_t lastStepAt = 0;
static uint8_t brightness = 128; // 0..255
static uint32_t lastAppMs = 0;
static bool appConnected = false;
static uint32_t lastBlink = 0;
static bool blinkOn = false;

static const uint8_t colors[][3] = {
  {255, 0, 0},   // red
  {255, 128, 0}, // orange
  {255, 255, 0}, // yellow
  {0, 255, 0},   // green
  {0, 255, 255}, // cyan
  {0, 0, 255},   // blue
  {128, 0, 255}, // purple
};
static const uint8_t colorCount = sizeof(colors) / sizeof(colors[0]);

static void setRgb(uint8_t idx) {
  const uint8_t* c = colors[idx % colorCount];
  // scale by brightness (0..255)
  uint8_t r = (uint8_t)((c[0] * brightness) / 255);
  uint8_t g = (uint8_t)((c[1] * brightness) / 255);
  uint8_t b = (uint8_t)((c[2] * brightness) / 255);
  rgbLedWrite(RGB_PIN, r, g, b);
}

void controlInit() {
  rearSetSpeed(0);
  steerStop();
  setRgb(colorIndex);
  lastAppMs = millis();
}

void controlApply(const ControlCommand& cmd) {
  lastCmd = cmd;
  rearSetSpeed(cmd.throttle);
  steerStart(cmd.steer, cmd.steerMs);

  // brightness from speed (0..100) -> 0..255
  if (cmd.speed >= 0 && cmd.speed <= 100) {
    brightness = (uint8_t)((cmd.speed * 255) / 100);
    setRgb(colorIndex);
  }

  // Step RGB ring when Up/Down or Left/Right pressed
  int stepInput = 0;
  if (cmd.throttle > 0) stepInput = 1;
  if (cmd.throttle < 0) stepInput = -1;
  if (stepInput == 0 && cmd.steer > 0) stepInput = 1;
  if (stepInput == 0 && cmd.steer < 0) stepInput = -1;

  const uint32_t now = millis();
  if (stepInput != 0 && stepInput != lastStepInput && (now - lastStepAt) > 150) {
    if (stepInput > 0) {
      colorIndex = (colorIndex + 1) % colorCount;
    } else {
      colorIndex = (colorIndex + colorCount - 1) % colorCount;
    }
    setRgb(colorIndex);
    lastStepAt = now;
  }
  lastStepInput = stepInput;

}

void controlLoop() {
  const uint32_t now = millis();
  if (now - lastAppMs > 2000) {
    appConnected = false;
  }

  if (!appConnected) {
    if (now - lastBlink >= 200) {
      lastBlink = now;
      blinkOn = !blinkOn;
      if (blinkOn) {
        rgbLedWrite(RGB_PIN, 64, 0, 0); // red blink
      } else {
        rgbLedWrite(RGB_PIN, 0, 0, 0);
      }
    }
  }
  steerLoop();
}

void controlNotifyAppActivity() {
  lastAppMs = millis();
  if (!appConnected) {
    appConnected = true;
    blinkOn = false;
    setRgb(colorIndex);
  }
}
