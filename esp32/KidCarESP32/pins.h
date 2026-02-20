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

// L298N (steering motor) - ENA is jumpered HIGH (no PWM on ENA)
static const int PIN_L298_ENA = 21; // Enable (kept HIGH)
static const int PIN_L298_IN1 = 10; // Dir
static const int PIN_L298_IN2 = 11; // Dir


// Manual selector + battery feedback inputs
static const int PIN_BATTERY_FB = 1;   // ADC, divider 100k/22k
static const int PIN_MANUAL_FWD = 14;  // Divider 10k/2.2k
static const int PIN_MANUAL_BACK = 13; // Divider 10k/2.2k
static const int PIN_MANUAL_THROTTLE = 12; // ADC, divider 100k/22k
// LEDC channels
static const int CH_BTS_R = 0;
static const int CH_BTS_L = 1;
static const int CH_L298  = 2;

void setupPins();
void setupPwm();

