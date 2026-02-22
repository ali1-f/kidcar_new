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

// Battery voltage calibration factor
// Calibrated with measured values: meter=13.13V, app=12.65V (ratio 1.037945).
static const float BATTERY_VOLT_CAL_FACTOR = 1.0142f;

// ACS758LCB-050B current sensor calibration (3.3V supply)
// Update these with real calibration points from your meter.
static const float CURRENT_SENSOR_ZERO_V = 1.650f;      // zero-current output voltage
static const float CURRENT_SENSOR_SENS_V_PER_A = 0.0264f; // ~26.4mV/A at 3.3V
static const float CURRENT_SENSOR_GAIN = 1.0f;          // scale correction
static const float CURRENT_SENSOR_OFFSET_A = 0.0f;      // offset correction
static const float CURRENT_SENSOR_DEADBAND_A = 0.20f;   // near-zero noise band

// Network settings
static const char* AP_SSID = "KidCar";
static const char* AP_PASS = "88958004";
static const uint16_t UDP_PORT = 4210;

// OTA settings (Wi-Fi firmware upload)
static const char* OTA_HOSTNAME = "kidcar-esp32";
static const char* OTA_PASSWORD = "kidcar123";
static const uint16_t OTA_PORT = 3232;

// RGB LED pin (common ESP32-S3 boards use 48, some use 38)
static const int RGB_PIN = 48;
