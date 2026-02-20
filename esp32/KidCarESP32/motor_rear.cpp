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
  static float currentDuty = 0.0f;
  static int currentDir = 0; // -1, 0, 1
  static uint32_t lastMs = 0;

  const uint32_t now = millis();
  if (lastMs == 0) lastMs = now;
  const uint32_t dt = now - lastMs;
  lastMs = now;

  int targetDir = 0;
  if (speed > 0) targetDir = 1;
  else if (speed < 0) targetDir = -1;

  const int minDuty = toDuty(REAR_SOFTSTART_MIN_PCT);

  if (targetDir == 0) {
    currentDuty = 0.0f;
    currentDir = 0;
  } else if (targetDir != currentDir) {
    // Direction change: stop first, then switch and start from min duty.
    currentDuty = (float)minDuty;
    currentDir = targetDir;
  } else if (currentDir != 0 && currentDuty <= 0.0f) {
    // Motion start: jump to minimum effective PWM first, then ramp.
    currentDuty = (float)minDuty;
  }

  int targetDuty = 0;
  if (targetDir != 0) {
    int pct = speed;
    if (pct < 0) pct = -pct;
    if (pct < REAR_SOFTSTART_MIN_PCT) pct = REAR_SOFTSTART_MIN_PCT;
    targetDuty = toDuty(pct);
  }

  // dt can be 0 on very fast loops; do not force a minimum step in that case.
  if (dt > 0) {
    const int maxDuty = (1 << PWM_RES) - 1;
    const float step = ((float)maxDuty * (float)dt) / (float)gRearRampMs;

    if (currentDuty < (float)targetDuty) {
      currentDuty += step;
      if (currentDuty > (float)targetDuty) currentDuty = (float)targetDuty;
    } else if (currentDuty > (float)targetDuty) {
      currentDuty -= step;
      if (currentDuty < (float)targetDuty) currentDuty = (float)targetDuty;
    }
  }

  int dutyOut = (int)(currentDuty + 0.5f);
  if (dutyOut < 0) dutyOut = 0;

  if (currentDir > 0) {
    ledcWriteChannel(CH_BTS_R, dutyOut);
    ledcWriteChannel(CH_BTS_L, 0);
  } else if (currentDir < 0) {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, dutyOut);
  } else {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, 0);
  }
}
