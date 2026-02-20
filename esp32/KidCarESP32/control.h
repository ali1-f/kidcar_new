#pragma once
#include "protocol.h"

void controlInit();
void controlApply(const ControlCommand& cmd);
void controlLoop();
void controlNotifyAppActivity();
float controlGetBatteryVoltage();
bool controlIsManualActive();
int8_t controlGetManualGear();
