#include "motor_steer.h"
#include "pins.h"
#include "config.h"

static uint32_t steerEndAt = 0;
static bool steerActive = false;
static bool steerNeedsRelease = false;
static int activeDir = 0;

static int toDuty(int pct) {
  if (pct < 0) pct = -pct;
  if (pct > 100) pct = 100;
  int maxDuty = (1 << PWM_RES) - 1;
  return map(pct, 0, 100, 0, maxDuty);
}

void steerStart(int direction, uint16_t durationMs) {
  const int dir = (direction > 0) ? 1 : (direction < 0 ? -1 : 0);

  // After timeout, force a button release (direction = 0) before allowing restart.
  if (dir == 0) {
    steerNeedsRelease = false;
    steerStop();
    return;
  }
  if (steerNeedsRelease) {
    steerStop();
    return;
  }

  if (STEER_MAX_MS > 0 && durationMs > STEER_MAX_MS) durationMs = STEER_MAX_MS;
  if (durationMs == 0) {
    steerStop();
    return;
  }

  // Keep one timing window for a continuous hold; do not extend on repeated packets.
  if (steerActive && dir == activeDir) {
    return;
  }

  if (dir > 0) {
    digitalWrite(PIN_L298_IN1, HIGH);
    digitalWrite(PIN_L298_IN2, LOW);
  } else {
    digitalWrite(PIN_L298_IN1, LOW);
    digitalWrite(PIN_L298_IN2, HIGH);
  }

  steerEndAt = millis() + durationMs;
  steerActive = true;
  activeDir = dir;
}

void steerStop() {
  digitalWrite(PIN_L298_IN1, LOW);
  digitalWrite(PIN_L298_IN2, LOW);
  steerActive = false;
  activeDir = 0;
}

void steerLoop() {
  if (STEER_MAX_MS > 0 && steerActive && millis() >= steerEndAt) {
    steerStop();
    steerNeedsRelease = true;
  }
}
