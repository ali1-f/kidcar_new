#include "control.h"
#include "motor_rear.h"
#include "motor_steer.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

static ControlCommand lastCmd = {0, 0, 0, 0, REAR_RAMP_MS, false};
static uint32_t lastAppMs = 0;
static bool appConnected = false;
static uint32_t lastBlink = 0;
static bool blinkOn = false;
static bool relayOn = false;
static uint32_t relayEnabledAt = 0;
static const uint32_t RELAY_DELAY_MS = 100;
static float batteryVoltage = 12.0f;
static bool manualActive = false;
static int8_t manualGear = 0; // -1 reverse, 0 neutral, 1 forward
static int8_t driveDir = 0;    // -1 reverse, 0 stop, 1 forward
static uint8_t driveSpeedPct = 0; // 0..100
static bool selectorFwdActive = false;
static bool selectorBackActive = false;
static float selectorThrottleVoltage = 0.0f;
static uint8_t selectorThrottlePct = 0;

static void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_PIN, r, g, b);
}

static void setRelay(bool enable) {
  if (enable == relayOn) return;
  relayOn = enable;
  digitalWrite(PIN_RELAY_EN, enable ? HIGH : LOW);
  if (enable) relayEnabledAt = millis();
}

static int readAdcAvg(int pin, uint8_t samples) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += (uint32_t)analogRead(pin);
  }
  return (int)(sum / samples);
}

static float readBatteryVoltageInstant() {
  const int raw = readAdcAvg(PIN_BATTERY_FB, 8);
  const float vAdc = ((float)raw * 3.3f) / 4095.0f;
  const float divider = (100.0f + 22.0f) / 22.0f;
  float vBat = vAdc * divider * BATTERY_VOLT_CAL_FACTOR;
  if (vBat < 0.0f) vBat = 0.0f;
  if (vBat > 20.0f) vBat = 20.0f;
  return vBat;
}

static int readManualThrottlePct() {
  const int raw = readAdcAvg(PIN_MANUAL_THROTTLE, 6);
  const float v = ((float)raw * 3.3f) / 4095.0f;

  // Manual throttle mapping:
  // >=2.0V: full stop
  // 1.4V: minimum speed
  // 0.0V: maximum speed
  if (v >= 2.0f) return 0;

  const int minPct = REAR_SOFTSTART_MIN_PCT;
  if (v <= 1.4f) {
    // 0.0..1.4V -> 100..minPct
    float t = v / 1.4f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int pct = (int)(100.0f - t * (100.0f - (float)minPct));
    if (pct < minPct) pct = minPct;
    if (pct > 100) pct = 100;
    return pct;
  }

  // 1.4..2.0V -> minPct..0 for smooth transition to stop
  float t = (v - 1.4f) / 0.6f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  int pct = (int)((1.0f - t) * (float)minPct);
  if (pct < 0) pct = 0;
  if (pct > minPct) pct = minPct;
  return pct;
}

static float readManualThrottleVoltage() {
  const int raw = readAdcAvg(PIN_MANUAL_THROTTLE, 6);
  return ((float)raw * 3.3f) / 4095.0f;
}

static ControlCommand resolveDriveCommand() {
  ControlCommand cmd = lastCmd;
  manualActive = (!appConnected) || lastCmd.manualMode;
  manualGear = 0;
  selectorFwdActive = false;
  selectorBackActive = false;
  selectorThrottleVoltage = 0.0f;
  selectorThrottlePct = 0;

  if (manualActive) {
    const float throttleV = readManualThrottleVoltage();
    selectorThrottleVoltage = throttleV;
    if (throttleV > 2.0f) {
      // Highest priority: if throttle input is above 2V, force full stop.
      manualGear = 0;
      selectorFwdActive = false;
      selectorBackActive = false;
      selectorThrottlePct = 0;
      cmd.throttle = 0;
      cmd.steer = 0;
      cmd.steerMs = STEER_MAX_MS;
      cmd.speed = 0;
      return cmd;
    }

    const bool fwd = digitalRead(PIN_MANUAL_FWD) == LOW;   // active-low
    const bool back = digitalRead(PIN_MANUAL_BACK) == LOW; // active-low
    selectorFwdActive = fwd;
    selectorBackActive = back;

    int dir = 0;
    if (fwd && !back) dir = 1;
    if (back && !fwd) dir = -1;

    int pct = readManualThrottlePct();
    selectorThrottlePct = (uint8_t)pct;
    // Keep full stop when throttle voltage commands 0%.
    // Soft-start minimum applies only for non-zero throttle requests.
    if (dir != 0 && pct > 0 && pct < REAR_SOFTSTART_MIN_PCT) {
      pct = REAR_SOFTSTART_MIN_PCT;
    }

    manualGear = (int8_t)dir;
    cmd.throttle = dir * pct;
    cmd.steer = 0;
    cmd.steerMs = STEER_MAX_MS;
    cmd.speed = pct;
  }

  return cmd;
}

void controlInit() {
  rearSetSpeed(0);
  steerStop();
  setRgb(0, 0, 0);
  lastAppMs = millis();
  setRelay(false);
  batteryVoltage = readBatteryVoltageInstant();
}

void controlApply(const ControlCommand& cmd) {
  lastCmd = cmd;
  rearSetRampMs(cmd.accelMs);
}

void controlLoop() {
  const uint32_t now = millis();
  if (now - lastAppMs > 2000) {
    appConnected = false;
  }

  // Smooth battery voltage for stable UI readout
  const float instantBattery = readBatteryVoltageInstant();
  batteryVoltage = (batteryVoltage * 0.85f) + (instantBattery * 0.15f);

  const ControlCommand cmd = resolveDriveCommand();
  if (cmd.throttle > 0) driveDir = 1;
  else if (cmd.throttle < 0) driveDir = -1;
  else driveDir = 0;
  int absThrottle = cmd.throttle;
  if (absThrottle < 0) absThrottle = -absThrottle;
  if (absThrottle > 100) absThrottle = 100;
  driveSpeedPct = (uint8_t)absThrottle;
  const bool wantMotion = (cmd.throttle != 0) || (cmd.steer != 0);

  if (manualActive) {
    if (wantMotion) {
      if (!relayOn) setRelay(true);
      if (relayOn && (now - relayEnabledAt) >= RELAY_DELAY_MS) {
        rearSetSpeed(cmd.throttle);
      } else {
        rearSetSpeed(0);
      }
    } else {
      rearSetSpeed(0);
      setRelay(false);
    }
    steerStop();
  } else {
    if (!appConnected) {
      setRelay(false);
      rearSetSpeed(0);
      steerStop();
    } else if (wantMotion) {
      if (!relayOn) {
        setRelay(true);
      }
      if (relayOn && (now - relayEnabledAt) >= RELAY_DELAY_MS) {
        rearSetSpeed(cmd.throttle);
        steerStart(cmd.steer, cmd.steerMs);
      } else {
        rearSetSpeed(0);
        steerStop();
      }
    } else {
      rearSetSpeed(0);
      steerStop();
      setRelay(false);
    }
  }

  // RGB status blink:
  // connected -> 500ms, disconnected -> 200ms
  const uint32_t blinkPeriod = appConnected ? 500 : 200;
  if (now - lastBlink >= blinkPeriod) {
    lastBlink = now;
    blinkOn = !blinkOn;
  }
  if (blinkOn) {
    if (driveSpeedPct == 0) {
      // Stopped: solid red during ON phase
      setRgb(255, 0, 0);
    } else {
      // Moving: brightness follows speed, color follows direction
      uint8_t v = (uint8_t)((driveSpeedPct * 255) / 100);
      if (v < 20) v = 20;
      if (driveDir > 0) {
        setRgb(0, v, 0);    // forward: green
      } else if (driveDir < 0) {
        setRgb(0, 0, v);    // reverse: blue
      } else {
        setRgb(255, 0, 0);  // fallback
      }
    }
  } else {
    setRgb(0, 0, 0);
  }

  steerLoop();
}

void controlNotifyAppActivity() {
  lastAppMs = millis();
  if (!appConnected) {
    appConnected = true;
    // Keep blink phase continuous; blink timing controls LED visibility.
  }
}

float controlGetBatteryVoltage() {
  return batteryVoltage;
}

bool controlIsManualActive() {
  return manualActive;
}

int8_t controlGetManualGear() {
  return manualGear;
}

int8_t controlGetDriveDir() {
  return driveDir;
}

uint8_t controlGetDriveSpeedPct() {
  return driveSpeedPct;
}

bool controlGetSelectorFwdActive() {
  return selectorFwdActive;
}

bool controlGetSelectorBackActive() {
  return selectorBackActive;
}

float controlGetSelectorThrottleVoltage() {
  return selectorThrottleVoltage;
}

uint8_t controlGetSelectorThrottlePct() {
  return selectorThrottlePct;
}
