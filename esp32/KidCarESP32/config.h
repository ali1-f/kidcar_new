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
static const uint16_t STEER_MAX_MS = 1200; // hard limit for steering motor run time

// Network settings
static const char* AP_SSID = "KidCar";
static const char* AP_PASS = "88958004";
static const uint16_t UDP_PORT = 4210;

// RGB LED pin (common ESP32-S3 boards use 48, some use 38)
static const int RGB_PIN = 48;
