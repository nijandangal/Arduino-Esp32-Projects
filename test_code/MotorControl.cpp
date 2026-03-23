#include "MotorControl.h"

MotorControl::MotorControl(int ena, int in1, int in2, int in3, int in4, int enb, int freq, int resolution) {
  ENA = ena; IN1 = in1; IN2 = in2; IN3 = in3; IN4 = in4; ENB = enb;
  motorSpeed = MIN_SPEED; 
  currentStatus = "Stopped";

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  if (!ledcAttach(ENA, freq, resolution)) Serial.println("Failed to attach ENA PWM");
  if (!ledcAttach(ENB, freq, resolution)) Serial.println("Failed to attach ENB PWM");

  stop();
}

void MotorControl::applyPwm() {
  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}