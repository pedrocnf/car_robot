#pragma once

#include <Arduino.h>

/**
 * @brief High-level motor controller for a differential drive car.
 *
 * The controller expects two DC motors driven by an H-bridge (e.g. L298N)
 * where each motor has a direction pair and a PWM enable pin.
 */
class CarController {
 public:
  CarController(uint8_t left_in1, uint8_t left_in2, uint8_t left_enable,
                uint8_t right_in1, uint8_t right_in2, uint8_t right_enable);

  /**
   * @brief Configure the controller pins and PWM timers.
   */
  void begin();

  void stop();
  void driveForward(uint8_t speed);
  void driveBackward(uint8_t speed);
  void turnLeft(uint8_t speed);
  void turnRight(uint8_t speed);

 private:
  void applyMotor(uint8_t pin_in1, uint8_t pin_in2, uint8_t enable_pin,
                  uint8_t channel, bool forward, uint8_t speed);

  uint8_t left_in1_;
  uint8_t left_in2_;
  uint8_t left_enable_;
  uint8_t right_in1_;
  uint8_t right_in2_;
  uint8_t right_enable_;

  static constexpr uint8_t LEFT_PWM_CHANNEL = 0;
  static constexpr uint8_t RIGHT_PWM_CHANNEL = 1;
  static constexpr uint32_t PWM_FREQUENCY = 20000;  // 20 KHz keeps motors quiet.
  static constexpr uint8_t PWM_RESOLUTION = 8;      // 0-255 speed values.
};
