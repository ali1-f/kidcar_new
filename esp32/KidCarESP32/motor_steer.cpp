#include "motor_steer.h"
#include "pins.h"
#include "config.h"

static uint32_t steerEndAt = 0;
static bool steerActive = false;

static int toDuty(int pct) {
  if (pct < 0) pct = -pct;
  if (pct > 100) pct = 100;
  int maxDuty = (1 << PWM_RES) - 1;
  return map(pct, 0, 100, 0, maxDuty);
}

void steerStart(int speed, uint16_t durationMs) {
  if (STEER_MAX_MS > 0 && durationMs > STEER_MAX_MS) durationMs = STEER_MAX_MS;
  if (durationMs == 0) {
    steerStop();
    return;
  }

  if (speed > 0) {
    digitalWrite(PIN_L298_IN1, HIGH);
    digitalWrite(PIN_L298_IN2, LOW);
  } else {
    digitalWrite(PIN_L298_IN1, LOW);
    digitalWrite(PIN_L298_IN2, HIGH);
  }

  steerEndAt = millis() + durationMs;
  steerActive = true;
}

void steerStop() {
  digitalWrite(PIN_L298_IN1, LOW);
  digitalWrite(PIN_L298_IN2, LOW);
  steerActive = false;
}

void steerLoop() {
  if (STEER_MAX_MS > 0 && steerActive && millis() >= steerEndAt) {
    steerStop();
  }
}
