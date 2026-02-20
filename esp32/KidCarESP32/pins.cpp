#include "pins.h"
#include "config.h"
#include <Arduino.h>
#include <esp32-hal-ledc.h>

#if TEST_BLINK
void setupPins() {}
void setupPwm() {}
#else
void setupPins() {
  // Hold relay OFF during boot (avoid floating)
  pinMode(PIN_RELAY_EN, INPUT_PULLDOWN);
  digitalWrite(PIN_RELAY_EN, LOW);
  pinMode(PIN_RELAY_EN, OUTPUT);

  pinMode(PIN_BTS_REN, OUTPUT);
  pinMode(PIN_BTS_LEN, OUTPUT);
  pinMode(PIN_L298_IN1, OUTPUT);
  pinMode(PIN_L298_IN2, OUTPUT);

  // Manual control inputs
  pinMode(PIN_MANUAL_FWD, INPUT);
  pinMode(PIN_MANUAL_BACK, INPUT);
  pinMode(PIN_MANUAL_THROTTLE, INPUT);
  pinMode(PIN_BATTERY_FB, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_MANUAL_THROTTLE, ADC_11db);
  analogSetPinAttenuation(PIN_BATTERY_FB, ADC_11db);

  digitalWrite(PIN_RELAY_EN, LOW);
  digitalWrite(PIN_BTS_REN, HIGH);
  digitalWrite(PIN_BTS_LEN, HIGH);
  digitalWrite(PIN_L298_IN1, LOW);
  digitalWrite(PIN_L298_IN2, LOW);
}

void setupPwm() {
  ledcAttachChannel(PIN_BTS_RPWM, PWM_FREQ, PWM_RES, CH_BTS_R);
  ledcAttachChannel(PIN_BTS_LPWM, PWM_FREQ, PWM_RES, CH_BTS_L);
  ledcAttachChannel(PIN_L298_ENA, PWM_FREQ, PWM_RES, CH_L298);
}
#endif
