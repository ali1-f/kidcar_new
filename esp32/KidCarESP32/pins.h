#pragma once

// ===== Pin mapping =====
// Fill these with your actual ESP32-S3 pins

// BTS7960 (rear motors)
static const int PIN_BTS_RPWM = 4;  // PWM
static const int PIN_BTS_LPWM = 5;  // PWM
static const int PIN_BTS_REN  = 17; // Enable
static const int PIN_BTS_LEN  = 18; // Enable

// Main power relay (enable)
static const int PIN_RELAY_EN = 16; // HIGH = enable

// L298N (steering motor)
static const int PIN_L298_ENA = 21; // PWM
static const int PIN_L298_IN1 = 38; // Dir
static const int PIN_L298_IN2 = 39; // Dir

// LEDC channels
static const int CH_BTS_R = 0;
static const int CH_BTS_L = 1;
static const int CH_L298  = 2;

void setupPins();
void setupPwm();
