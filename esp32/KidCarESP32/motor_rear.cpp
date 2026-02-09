#include "motor_rear.h"
#include "pins.h"
#include "config.h"

static int toDuty(int pct) {
  if (pct < 0) pct = -pct;
  if (pct > 100) pct = 100;
  int maxDuty = (1 << PWM_RES) - 1;
  return map(pct, 0, 100, 0, maxDuty);
}

void rearSetSpeed(int speed) {
  int duty = toDuty(speed);
  if (speed > 0) {
    ledcWriteChannel(CH_BTS_R, duty);
    ledcWriteChannel(CH_BTS_L, 0);
  } else if (speed < 0) {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, duty);
  } else {
    ledcWriteChannel(CH_BTS_R, 0);
    ledcWriteChannel(CH_BTS_L, 0);
  }
}
