#include "CarController.h"

CarController::CarController(uint8_t left_in1, uint8_t left_in2,
                             uint8_t left_enable, uint8_t right_in1,
                             uint8_t right_in2, uint8_t right_enable)
    : left_in1_(left_in1),
      left_in2_(left_in2),
      left_enable_(left_enable),
      right_in1_(right_in1),
      right_in2_(right_in2),
      right_enable_(right_enable) {}

void CarController::begin() {
  pinMode(left_in1_, OUTPUT);
  pinMode(left_in2_, OUTPUT);
  pinMode(right_in1_, OUTPUT);
  pinMode(right_in2_, OUTPUT);

  ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(left_enable_, LEFT_PWM_CHANNEL);
  ledcAttachPin(right_enable_, RIGHT_PWM_CHANNEL);

  stop();
}

void CarController::applyMotor(uint8_t pin_in1, uint8_t pin_in2, uint8_t enable_pin,
                               uint8_t channel, bool forward, uint8_t speed) {
  digitalWrite(pin_in1, forward ? HIGH : LOW);
  digitalWrite(pin_in2, forward ? LOW : HIGH);
  ledcWrite(channel, speed);
}

void CarController::stop() {
  digitalWrite(left_in1_, LOW);
  digitalWrite(left_in2_, LOW);
  digitalWrite(right_in1_, LOW);
  digitalWrite(right_in2_, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
}

void CarController::driveForward(uint8_t speed) {
  applyMotor(left_in1_, left_in2_, left_enable_, LEFT_PWM_CHANNEL, true, speed);
  applyMotor(right_in1_, right_in2_, right_enable_, RIGHT_PWM_CHANNEL, true, speed);
}

void CarController::driveBackward(uint8_t speed) {
  applyMotor(left_in1_, left_in2_, left_enable_, LEFT_PWM_CHANNEL, false, speed);
  applyMotor(right_in1_, right_in2_, right_enable_, RIGHT_PWM_CHANNEL, false, speed);
}

void CarController::turnLeft(uint8_t speed) {
  applyMotor(left_in1_, left_in2_, left_enable_, LEFT_PWM_CHANNEL, false, speed);
  applyMotor(right_in1_, right_in2_, right_enable_, RIGHT_PWM_CHANNEL, true, speed);
}

void CarController::turnRight(uint8_t speed) {
  applyMotor(left_in1_, left_in2_, left_enable_, LEFT_PWM_CHANNEL, true, speed);
  applyMotor(right_in1_, right_in2_, right_enable_, RIGHT_PWM_CHANNEL, false, speed);
}
