# Flask controller

Small Flask REST API that publishes MQTT commands for the ESP32 based car.

## Endpoints

- `POST /api/control` – JSON body with `action` (`forward`, `backward`, `left`,
  `right`, `stop`) and optional `speed` (0-255). Returns the normalized payload.
- `GET /health` – returns `{"status": "ready"}` and can be used for health checks.

## Running locally

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
flask --app app run --host 0.0.0.0 --port 5000
```

Ensure the development machine is connected to the ESP32 Wi-Fi network so the
MQTT packets can reach the broker.
