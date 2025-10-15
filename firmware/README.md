# ESP32 Firmware

This PlatformIO project turns the ESP-WROOM-32 into a self-hosted MQTT broker.
It also manages a dual motor driver that propels the car.

## Pinout

The default mapping targets a generic ESP32 DevKit board connected to an L298N
module. Update the constants at the top of `src/main.cpp` if your wiring
differs.

| Function          | GPIO |
| ----------------- | ---- |
| Left motor IN1    | 27   |
| Left motor IN2    | 26   |
| Left motor enable | 25   |
| Right motor IN1   | 33   |
| Right motor IN2   | 32   |
| Right motor enable| 14   |

## MQTT protocol support

The broker supports a single client at a time and implements a subset of the
MQTT 3.1.1 specification:

- `CONNECT`, `CONNACK`
- `PUBLISH` (QoS 0 or 1)
- `PINGREQ`, `PINGRESP`
- `DISCONNECT`

Messages published to `car/control` are forwarded to the `CarController`
component that manipulates the motors.
