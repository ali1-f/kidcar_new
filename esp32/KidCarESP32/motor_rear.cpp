#include "motor_rear.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

static uint16_t gRearRampMs = REAR_RAMP_MS;

static int toDuty(int pct) {
  if (pct < 0) pct = -pct;
  if (pct > 100) pct = 100;
  int maxDuty = (1 << PWM_RES) - 1;
  return map(pct, 0, 100, 0, maxDuty);
}

void rearSetRampMs(uint16_t rampMs) {
  if (rampMs < 100) rampMs = 100;
  if (rampMs > 5000) rampMs = 5000;
  gRearRampMs = rampMs;
}

void rearSetSpeed(int speed) {
  static int currentDuty = 0;
  static int currentDir = 0; // -1, 0, 1
  static uint32_t lastMs = 0;

  const uint32_t now = millis();
  if (lastMs == 0) lastMs = now;
  uint32_t dt = now - lastMs;
  lastMs = now;

  int targetDir = 0;
  if (speed > 0) targetDir = 1;
  else if (speed < 0) targetDir = -1;

  if (targetDir == 0) {
    currentDuty = 0;
    currentDir = 0;
  } else if (targetDir != currentDir) {
    // direction change: stop first, then switch
    currentDuty = 0;
    currentDir = targetDir;
  }

  int targetDuty = 0;
  if (targetDir != 0) {
    int pct = speed;
    if (pct < 0) pct = -pct;
    if (pct < REAR_SOFTSTART_MIN_PCT) pct = REAR_SOFTSTART_MIN_PCT;
    targetDuty = toDuty(pct);
  }

  if (currentDuty < targetDuty) {
    int maxDuty = (1 << PWM_RES) - 1;
    int step = (int)((maxDuty * (uint32_t)dt) / (uint32_t)gRearRampMs);
    if (step < 1) step = 1;
    currentDuty += step;
    if (currentDuty > targetDuty) currentDuty = targetDuty;
  } else if (currentDuty > targetDuty) {
    int maxDuty = (1 << PWM_RES) - 1;
    int step = (int)((maxDuty * (uint32_t)dt) / (uint32_t)gRearRampMs);
    if (step < 1) step = 1;
    currentDuty -= step;
    if (currentDuty < targetDuty) currentDuty = targetDuty;
  }

  if (currentDir > 0) {
    ledcWriteChannel(CH_BTS_R, currentDuty);
    ledcWriteChannel(CH_BTS_L, 0);
  } else if (currentDir < 0) {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, currentDuty);
  } else {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, 0);
  }
}
