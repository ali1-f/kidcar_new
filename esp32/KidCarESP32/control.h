#pragma once
#include "protocol.h"

void controlInit();
void controlApply(const ControlCommand& cmd);
void controlLoop();
void controlNotifyAppActivity();
float controlGetBatteryVoltage();
float controlGetCurrentAmp();
float controlGetCurrentOu1Voltage();
float controlGetCurrentOu2Voltage();
bool controlIsManualActive();
int8_t controlGetManualGear();
int8_t controlGetDriveDir();
uint8_t controlGetDriveSpeedPct();
bool controlGetSelectorFwdActive();
bool controlGetSelectorBackActive();
float controlGetSelectorThrottleVoltage();
uint8_t controlGetSelectorThrottlePct();
