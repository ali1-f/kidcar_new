#include "protocol.h"
#include "config.h"
#include <ArduinoJson.h>

bool protocolParse(const char* msg, ControlCommand& out) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) return false;

  out.throttle = doc["throttle"] | 0;
  out.steer    = doc["steer"] | 0;
  out.steerMs  = doc["steer_ms"] | 0;
  out.speed    = doc["speed"] | 0;

  if (out.throttle > 100) out.throttle = 100;
  if (out.throttle < -100) out.throttle = -100;
  if (out.steer > 100) out.steer = 100;
  if (out.steer < -100) out.steer = -100;
  if (out.steerMs > STEER_MAX_MS) out.steerMs = STEER_MAX_MS;
  if (out.speed > 100) out.speed = 100;
  if (out.speed < 0) out.speed = 0;

  return true;
}
