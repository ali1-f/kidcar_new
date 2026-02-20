#pragma once
#include <Arduino.h>

// ===== Project configuration =====

// ===== Temporary test mode =====
// 1 = blink LED only, 0 = normal app
#define TEST_BLINK 0

// PWM configuration for ESP32-S3
static const int PWM_FREQ = 20000; // 20kHz
static const int PWM_RES  = 10;    // 0..1023

// Steering safety
static const uint16_t STEER_MAX_MS = 5000; // hard limit for steering motor run time
// PWM for steering is disabled (ENA jumpered). Only time limit applies.
static const uint8_t STEER_MAX_PWM_PCT = 60; // unused when PWM disabled

// Rear motor soft-start
static const uint8_t REAR_SOFTSTART_MIN_PCT = 20; // safe start percent
static const uint16_t REAR_RAMP_MS = 600; // time to ramp to target

// Battery voltage calibration factor (disabled: ADC pin calibration is used)
// Set to 1.0f to avoid double calibration.
static const float BATTERY_VOLT_CAL_FACTOR = 1.0000f;

// Network settings
static const char* AP_SSID = "KidCar";
static const char* AP_PASS = "88958004";
static const uint16_t UDP_PORT = 4210;

// RGB LED pin (common ESP32-S3 boards use 48, some use 38)
static const int RGB_PIN = 48;




