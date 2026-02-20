#pragma once
#include <Arduino.h>

struct ControlCommand {
  int throttle;     // -100..100
  int steer;        // -100..100
  uint16_t steerMs; // 0..STEER_MAX_MS
  int speed;        // 0..100
  uint16_t accelMs; // rear PWM ramp time (ms)
  bool manualMode;  // true = use hardware manual inputs
  bool park;        // true = movement lock
  uint8_t reverseSpeed; // 0..100 max reverse speed
};

bool protocolParse(const char* msg, ControlCommand& out);


