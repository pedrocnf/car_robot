import atexit
import logging
import os
from typing import Any, Dict, Optional

from dotenv import load_dotenv
from flask import Flask, jsonify, request

from mqtt_client import CarMqttClient

logging.basicConfig(level=logging.INFO)
LOGGER = logging.getLogger(__name__)

load_dotenv()

MQTT_HOST = os.getenv("MQTT_BROKER_HOST", "192.168.4.1")
MQTT_PORT = int(os.getenv("MQTT_BROKER_PORT", "1883"))

app = Flask(__name__)
client = CarMqttClient(MQTT_HOST, MQTT_PORT)


@app.before_first_request
def _connect_mqtt() -> None:
    LOGGER.info("Connecting to MQTT broker at %s:%s", MQTT_HOST, MQTT_PORT)
    client.connect()


def _validate_payload(data: Dict[str, Any]) -> Dict[str, Any]:
    if "action" not in data:
        raise ValueError("'action' is required")
    action = str(data["action"]).lower()
    if action not in {"forward", "backward", "left", "right", "stop"}:
        raise ValueError(
            "action must be one of 'forward', 'backward', 'left', 'right', 'stop'"
        )
    speed: Optional[int] = None
    if "speed" in data and data["speed"] is not None:
        speed = int(data["speed"])
        if speed < 0 or speed > 255:
            raise ValueError("speed must be between 0 and 255")
    return {"action": action, "speed": speed}


@app.route("/api/control", methods=["POST"])
def control() -> Any:
    data = request.get_json(silent=True)
    if not isinstance(data, dict):
        return jsonify({"error": "JSON body required"}), 400
    try:
        payload = _validate_payload(data)
        client.publish_command(payload["action"], payload["speed"])
    except (ValueError, RuntimeError, ConnectionError) as exc:
        LOGGER.exception("Failed to send command")
        return jsonify({"error": str(exc)}), 400
    return jsonify({"status": "ok", **payload})


@app.route("/health", methods=["GET"])
def health() -> Any:
    return jsonify({"status": "ready"})


def _shutdown() -> None:
    client.stop()


atexit.register(_shutdown)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.getenv("PORT", "5000")))
