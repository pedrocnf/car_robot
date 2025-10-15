import logging
import threading
from typing import Optional

import paho.mqtt.client as mqtt

LOGGER = logging.getLogger(__name__)


class CarMqttClient:
    """Light-weight MQTT publisher for sending commands to the ESP32 broker."""

    def __init__(self, host: str, port: int = 1883, keepalive: int = 60) -> None:
        self._host = host
        self._port = port
        self._keepalive = keepalive
        self._client = mqtt.Client()
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_publish = self._on_publish
        self._loop_thread: Optional[threading.Thread] = None
        self._connected = threading.Event()

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            LOGGER.info("Connected to MQTT broker at %s:%s", self._host, self._port)
            self._connected.set()
        else:
            LOGGER.error("Failed to connect to MQTT broker (rc=%s)", rc)

    def _on_disconnect(self, client, userdata, rc):
        LOGGER.warning("Disconnected from MQTT broker (rc=%s)", rc)
        self._connected.clear()

    def _on_publish(self, client, userdata, mid):
        LOGGER.debug("Published MQTT message with id %s", mid)

    def connect(self) -> None:
        if self._loop_thread and self._loop_thread.is_alive():
            return

        self._client.connect(self._host, self._port, self._keepalive)
        self._loop_thread = threading.Thread(target=self._client.loop_forever, daemon=True)
        self._loop_thread.start()
        if not self._connected.wait(timeout=5):
            raise ConnectionError(
                f"Timed out connecting to MQTT broker at {self._host}:{self._port}"
            )

    def publish_command(self, action: str, speed: Optional[int] = None) -> None:
        if speed is not None and (speed < 0 or speed > 255):
            raise ValueError("speed must be between 0 and 255")
        payload = action if speed is None else f"{action}:{speed}"
        LOGGER.info("Publishing control command '%s'", payload)
        result = self._client.publish("car/control", payload, qos=0, retain=False)
        status = result.rc
        if status != mqtt.MQTT_ERR_SUCCESS:
            raise RuntimeError(f"Failed to publish command: rc={status}")

    def stop(self) -> None:
        if self._loop_thread and self._loop_thread.is_alive():
            LOGGER.info("Stopping MQTT client loop")
            self._client.disconnect()
            self._loop_thread.join(timeout=2)
            self._loop_thread = None


__all__ = ["CarMqttClient"]
