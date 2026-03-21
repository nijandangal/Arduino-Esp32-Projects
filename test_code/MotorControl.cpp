#include "MotorControl.h"

MotorControl::MotorControl(int ena, int in1, int in2, int in3, int in4, int enb, int freq, int resolution) {
  ENA = ena; IN1 = in1; IN2 = in2; IN3 = in3; IN4 = in4; ENB = enb;
  motorSpeed = MIN_SPEED; 
  currentStatus = "Stopped";

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  // ESP32 Core 3.3.7 new style
  if (!ledcAttach(ENA, freq, resolution)) {
    Serial.println("Failed to attach ENA PWM");
  }
  if (!ledcAttach(ENB, freq, resolution)) {
    Serial.println("Failed to attach ENB PWM");
  }

  stop();
}

void MotorControl::applyPwm() {
  ledcWrite(ENA, motorSpeed);
  ledcWrite(ENB, motorSpeed);
}

void MotorControl::setSpeed(int speed) {
  motorSpeed = constrain(speed, MIN_SPEED, MAX_SPEED);
  applyPwm();
  currentStatus = "Speed set to " + String(motorSpeed);
}

void MotorControl::stop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  currentStatus = "Stopped";
}

void MotorControl::forward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  applyPwm();
  currentStatus = "Forward";
}

void MotorControl::backward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  applyPwm();
  currentStatus = "Backward";
}

void MotorControl::left() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  applyPwm();
  currentStatus = "Left";
}

void MotorControl::right() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  applyPwm();
  currentStatus = "Right";
}

int MotorControl::getSpeed() { return motorSpeed; }
String MotorControl::getStatus() { return currentStatus; }