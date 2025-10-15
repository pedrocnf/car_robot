# Car Robot MQTT Platform

This repository contains a reference implementation for a differential drive car
based on the ESP-WROOM-32 module. The ESP32 exposes its own Wi-Fi access point
and runs a minimalist MQTT broker that accepts driving commands from a Python
web application built with Flask.

## Repository layout

- `firmware/` – PlatformIO project for the ESP32 firmware that implements the
  MQTT broker and controls the motor driver.
- `python_app/` – Flask application that exposes a REST endpoint and publishes
  MQTT commands to the car.

## Firmware overview

The firmware is written for the Arduino framework using PlatformIO. It creates a
Wi-Fi access point (`CarRobot`) so that a controller device can connect directly
to the vehicle. A lightweight MQTT broker runs on port `1883`; incoming
`PUBLISH` messages on the `car/control` topic are translated into motor
operations.

Supported commands:

| Payload         | Description                        |
| --------------- | ---------------------------------- |
| `forward:200`   | Drive forward with speed 200/255.  |
| `backward:150`  | Drive backward with speed 150.     |
| `left:180`      | Turn in place toward the left.     |
| `right:180`     | Turn in place toward the right.    |
| `stop`          | Immediately stop both motors.      |

If the speed suffix is omitted the default duty cycle `180` is used. The PWM
frequency is set to 20 kHz to minimize audible noise.

### Building and flashing

1. Install [PlatformIO Core](https://platformio.org/install/cli).
2. Connect the ESP32 board and update the pin assignments in
   `firmware/src/main.cpp` if necessary.
3. From the `firmware/` directory, build and upload the project:

   ```bash
   pio run --target upload
   ```

4. Use the serial monitor to observe logs:

   ```bash
   pio device monitor
   ```

After boot the serial log prints the access point IP address (default
`192.168.4.1`).

## Python controller

The controller provides a REST API that publishes MQTT messages to the ESP32
broker. It is useful for quick testing or for integration with a higher level
system.

### Setup

```bash
cd python_app
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env  # adjust the broker IP if required
flask --app app run --host 0.0.0.0 --port 5000
```

### Usage

Send driving commands with any HTTP client while connected to the ESP32 access
point:

```bash
curl -X POST http://localhost:5000/api/control \
  -H "Content-Type: application/json" \
  -d '{"action": "forward", "speed": 200}'
```

The service responds with the normalized command payload. A health endpoint is
available at `GET /health`.

## Safety considerations

- Always elevate the car on a stand when testing motor control to avoid
  unintended movement.
- Validate wiring to the motor driver (e.g. L298N, TB6612FNG) and confirm that
  the selected GPIO pins support PWM.
- The bundled MQTT broker accepts a single client connection at a time and is
  intended for local control only.
