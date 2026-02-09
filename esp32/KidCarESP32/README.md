# KidCar ESP32 (Arduino)

This sketch is organized for clean growth.

Structure:
- KidCarESP32.ino: entry point
- config.h: app-level constants
- pins.h: pin mapping
- protocol.h/.cpp: command parsing
- motor_rear.h/.cpp: rear motor control (BTS7960)
- motor_steer.h/.cpp: steering motor control (L298N)
- wifi_ap.h/.cpp: AP mode + network server
- control.h/.cpp: central control logic

Next steps:
1) Fill pin numbers in `pins.h`.
2) Choose protocol (UDP / WebSocket / HTTP) and implement in `wifi_ap.*` + `protocol.*`.
3) Install `ArduinoJson` library (used in `protocol.cpp`).
