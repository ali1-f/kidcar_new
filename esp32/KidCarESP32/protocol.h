#pragma once
#include <Arduino.h>

struct ControlCommand {
  int throttle;    // -100..100
  int steer;       // -100..100
  uint16_t steerMs; // 0..STEER_MAX_MS
  int speed;       // 0..100
};

bool protocolParse(const char* msg, ControlCommand& out);
