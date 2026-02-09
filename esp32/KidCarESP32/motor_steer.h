#pragma once
#include <Arduino.h>

void steerStart(int direction, uint16_t durationMs);
void steerStop();
void steerLoop();
