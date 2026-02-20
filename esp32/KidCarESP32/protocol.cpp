#include "protocol.h"
#include "config.h"
#include <ArduinoJson.h>
#include <string.h>

bool protocolParse(const char* msg, ControlCommand& out) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return false;

  out.throttle = doc["throttle"] | 0;
  out.steer = doc["steer"] | 0;
  out.steerMs = doc["steer_ms"] | STEER_MAX_MS;
  out.speed = doc["speed"] | 0;
  out.accelMs = doc["accel_ms"] | REAR_RAMP_MS;
  out.manualMode = doc["manual"] | false;
  out.park = doc["park"] | false;

  if (doc["mode"].is<const char*>()) {
    const char* mode = doc["mode"];
    if (mode != nullptr) {
      if (strcmp(mode, "manual") == 0 || strcmp(mode, "MANUAL") == 0) {
        out.manualMode = true;
      } else if (strcmp(mode, "remote") == 0 || strcmp(mode, "REMOTE") == 0) {
        out.manualMode = false;
      }
    }
  }

  if (out.throttle > 100) out.throttle = 100;
  if (out.throttle < -100) out.throttle = -100;
  if (out.steer > 100) out.steer = 100;
  if (out.steer < -100) out.steer = -100;
  if (out.steerMs > STEER_MAX_MS) out.steerMs = STEER_MAX_MS;
  if (out.steer != 0 && out.steerMs == 0) out.steerMs = STEER_MAX_MS;
  if (out.speed > 100) out.speed = 100;
  if (out.speed < 0) out.speed = 0;
  if (out.accelMs < 100) out.accelMs = 100;
  if (out.accelMs > 5000) out.accelMs = 5000;

  return true;
}
