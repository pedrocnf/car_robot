#include <Arduino.h>
#include <WiFi.h>

#include "CarController.h"

// Wi-Fi credentials for the car access point.
constexpr const char* WIFI_SSID = "CarRobot";
constexpr const char* WIFI_PASSWORD = "carrobot123";

constexpr uint16_t MQTT_PORT = 1883;
constexpr const char* CONTROL_TOPIC = "car/control";

// Motor pin mapping (tune to match your wiring).
constexpr uint8_t LEFT_IN1 = 27;
constexpr uint8_t LEFT_IN2 = 26;
constexpr uint8_t LEFT_ENABLE = 25;
constexpr uint8_t RIGHT_IN1 = 33;
constexpr uint8_t RIGHT_IN2 = 32;
constexpr uint8_t RIGHT_ENABLE = 14;

WiFiServer mqttServer(MQTT_PORT);
WiFiClient mqttClient;
CarController car(LEFT_IN1, LEFT_IN2, LEFT_ENABLE, RIGHT_IN1, RIGHT_IN2,
                  RIGHT_ENABLE);

uint16_t lastPacketIdentifier = 0;

bool readRemainingLength(WiFiClient& client, size_t& value) {
  value = 0;
  uint32_t multiplier = 1;
  for (int i = 0; i < 4; ++i) {
    // Wait for data to arrive.
    unsigned long start = millis();
    while (!client.available()) {
      if (!client.connected()) {
        return false;
      }
      if (millis() - start > 1000) {
        return false;
      }
      delay(1);
    }

    int encoded = client.read();
    if (encoded < 0) {
      return false;
    }
    value += static_cast<size_t>(encoded & 0x7F) * multiplier;
    if ((encoded & 0x80) == 0) {
      return true;
    }
    multiplier *= 128;
  }
  return false;
}

bool readExact(WiFiClient& client, uint8_t* buffer, size_t length) {
  size_t read = client.readBytes(buffer, length);
  return read == length;
}

void skipBytes(WiFiClient& client, size_t length) {
  uint8_t scratch[16];
  while (length > 0) {
    size_t chunk = length > sizeof(scratch) ? sizeof(scratch) : length;
    size_t read = client.readBytes(scratch, chunk);
    if (read == 0) {
      break;
    }
    length -= read;
  }
}

uint8_t clampSpeed(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return static_cast<uint8_t>(value);
}

void executeCommand(const String& payload) {
  String command = payload;
  command.trim();
  if (command.length() == 0) {
    return;
  }

  int colon = command.indexOf(':');
  String action = command;
  uint8_t speed = 180;  // default speed.
  if (colon >= 0) {
    action = command.substring(0, colon);
    String speedPart = command.substring(colon + 1);
    speedPart.trim();
    speed = clampSpeed(speedPart.toInt());
  }
  action.toLowerCase();

  if (action == "forward") {
    car.driveForward(speed);
    Serial.printf("Driving forward at speed %u\n", speed);
  } else if (action == "backward") {
    car.driveBackward(speed);
    Serial.printf("Driving backward at speed %u\n", speed);
  } else if (action == "left") {
    car.turnLeft(speed);
    Serial.printf("Turning left at speed %u\n", speed);
  } else if (action == "right") {
    car.turnRight(speed);
    Serial.printf("Turning right at speed %u\n", speed);
  } else if (action == "stop") {
    car.stop();
    Serial.println("Stopping motors");
  } else {
    Serial.printf("Unknown command: %s\n", action.c_str());
  }
}

void handlePublish(uint8_t header, size_t remainingLength) {
  uint8_t buffer[2];
  if (!readExact(mqttClient, buffer, sizeof(buffer))) {
    return;
  }
  remainingLength -= sizeof(buffer);
  uint16_t topicLength = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];

  String topic;
  topic.reserve(topicLength);
  for (uint16_t i = 0; i < topicLength; ++i) {
    int value = mqttClient.read();
    if (value < 0) {
      return;
    }
    topic += static_cast<char>(value);
  }
  remainingLength -= topicLength;

  uint8_t qos = (header & 0x06) >> 1;
  uint16_t packetId = 0;
  if (qos > 0) {
    if (!readExact(mqttClient, buffer, sizeof(buffer))) {
      return;
    }
    remainingLength -= sizeof(buffer);
    packetId = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
    lastPacketIdentifier = packetId;
  }

  String payload;
  payload.reserve(remainingLength);
  for (size_t i = 0; i < remainingLength; ++i) {
    int value = mqttClient.read();
    if (value < 0) {
      return;
    }
    payload += static_cast<char>(value);
  }

  Serial.printf("Received MQTT message on %s: %s\n", topic.c_str(),
                payload.c_str());

  if (topic == CONTROL_TOPIC) {
    executeCommand(payload);
  }

  if (qos == 1 && packetId != 0) {
    uint8_t puback[] = {0x40, 0x02, static_cast<uint8_t>(packetId >> 8),
                        static_cast<uint8_t>(packetId & 0xFF)};
    mqttClient.write(puback, sizeof(puback));
  }
}

void handleConnect(size_t remainingLength) {
  skipBytes(mqttClient, remainingLength);
  const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
  mqttClient.write(connack, sizeof(connack));
  Serial.println("MQTT client connected");
}

void handlePingRequest() {
  const uint8_t pingresp[] = {0xD0, 0x00};
  mqttClient.write(pingresp, sizeof(pingresp));
}

void processMqttClient() {
  while (mqttClient.connected() && mqttClient.available()) {
    int header = mqttClient.read();
    if (header < 0) {
      return;
    }
    uint8_t packetType = (header >> 4) & 0x0F;

    size_t remainingLength = 0;
    if (!readRemainingLength(mqttClient, remainingLength)) {
      return;
    }

    switch (packetType) {
      case 1:  // CONNECT
        handleConnect(remainingLength);
        break;
      case 3:  // PUBLISH
        handlePublish(header, remainingLength);
        break;
      case 12:  // PINGREQ
        skipBytes(mqttClient, remainingLength);
        handlePingRequest();
        break;
      case 14:  // DISCONNECT
        skipBytes(mqttClient, remainingLength);
        mqttClient.stop();
        Serial.println("MQTT client disconnected");
        break;
      default:
        Serial.printf("Unsupported MQTT packet type: %u\n", packetType);
        skipBytes(mqttClient, remainingLength);
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting car robot MQTT broker...");

  car.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("Access point ready. Connect to SSID '%s' and MQTT broker %s:%u\n",
                WIFI_SSID, ip.toString().c_str(), MQTT_PORT);

  mqttServer.begin();
}

void loop() {
  if (!mqttClient.connected()) {
    if (mqttServer.hasClient()) {
      WiFiClient candidate = mqttServer.available();
      if (candidate) {
        if (mqttClient && mqttClient.connected()) {
          candidate.stop();
        } else {
          mqttClient = candidate;
        }
      }
    }
  }

  if (mqttClient && mqttClient.connected()) {
    processMqttClient();
  }
}
