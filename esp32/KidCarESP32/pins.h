#pragma once

// ===== Pin mapping =====
// Fill these with your actual ESP32-S3 pins

// BTS7960 (rear motors)
static const int PIN_BTS_RPWM = 2; // PWM
static const int PIN_BTS_LPWM = 3; // PWM
static const int PIN_BTS_REN  = 4; // Enable
static const int PIN_BTS_LEN  = 5; // Enable

// L298N (steering motor)
static const int PIN_L298_ENA = 6; // PWM
static const int PIN_L298_IN1 = 7; // Dir
static const int PIN_L298_IN2 = 8; // Dir

// LEDC channels
static const int CH_BTS_R = 0;
static const int CH_BTS_L = 1;
static const int CH_L298  = 2;

void setupPins();
void setupPwm();
