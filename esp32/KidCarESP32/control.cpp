#include "control.h"
#include "motor_rear.h"
#include "motor_steer.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

static ControlCommand lastCmd = {0, 0, 0, 0, REAR_RAMP_MS, false};
static uint8_t colorIndex = 0;
static int lastStepInput = 0;
static uint32_t lastStepAt = 0;
static uint8_t brightness = 128; // 0..255
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
  uint8_t r = (uint8_t)((c[0] * brightness) / 255);
  uint8_t g = (uint8_t)((c[1] * brightness) / 255);
  uint8_t b = (uint8_t)((c[2] * brightness) / 255);
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
  int pct = map(raw, 100, 2800, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

static ControlCommand resolveDriveCommand() {
  ControlCommand cmd = lastCmd;
  manualActive = (!appConnected) || lastCmd.manualMode;
  manualGear = 0;

  if (manualActive) {
    const bool fwd = digitalRead(PIN_MANUAL_FWD) == HIGH;
    const bool back = digitalRead(PIN_MANUAL_BACK) == HIGH;

    int dir = 0;
    if (fwd && !back) dir = 1;
    if (back && !fwd) dir = -1;

    int pct = readManualThrottlePct();
    if (dir < 0) {
      // Requirement: reverse speed in manual mode must be fixed at 50%.
      pct = 50;
    }
    if (dir != 0 && pct < REAR_SOFTSTART_MIN_PCT) {
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
  setRgb(colorIndex);
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
  const bool wantMotion = (cmd.throttle != 0) || (cmd.steer != 0);

  if (manualActive) {
    blinkOn = false;
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
      if (now - lastBlink >= 200) {
        lastBlink = now;
        blinkOn = !blinkOn;
        if (blinkOn) {
          rgbLedWrite(RGB_PIN, 64, 0, 0);
        } else {
          rgbLedWrite(RGB_PIN, 0, 0, 0);
        }
      }
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

  if (cmd.speed >= 0 && cmd.speed <= 100) {
    brightness = (uint8_t)((cmd.speed * 255) / 100);
    if (!(manualActive == false && appConnected == false && blinkOn)) {
      setRgb(colorIndex);
    }
  }

  int stepInput = 0;
  if (cmd.throttle > 0) stepInput = 1;
  if (cmd.throttle < 0) stepInput = -1;
  if (stepInput == 0 && cmd.steer > 0) stepInput = 1;
  if (stepInput == 0 && cmd.steer < 0) stepInput = -1;

  if (stepInput != 0 && stepInput != lastStepInput && (now - lastStepAt) > 150) {
    if (stepInput > 0) {
      colorIndex = (colorIndex + 1) % colorCount;
    } else {
      colorIndex = (colorIndex + colorCount - 1) % colorCount;
    }
    if (!(manualActive == false && appConnected == false && blinkOn)) {
      setRgb(colorIndex);
    }
    lastStepAt = now;
  }
  lastStepInput = stepInput;

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

float controlGetBatteryVoltage() {
  return batteryVoltage;
}

bool controlIsManualActive() {
  return manualActive;
}

int8_t controlGetManualGear() {
  return manualGear;
}
