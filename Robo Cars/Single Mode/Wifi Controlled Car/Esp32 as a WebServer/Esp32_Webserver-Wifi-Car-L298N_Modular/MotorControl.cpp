#include "MotorControl.h"

MotorControl::MotorControl(int ena, int in1, int in2, int in3, int in4, int enb, int freq, int resolution) {
  ENA = ena; IN1 = in1; IN2 = in2; IN3 = in3; IN4 = in4; ENB = enb;
  channelA = 0; channelB = 1;
  motorSpeed = 0;
  currentStatus = "Stopped";

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  ledcAttachChannel(ENA, freq, resolution, channelA);
  ledcAttachChannel(ENB, freq, resolution, channelB);

  stop(); // ensure motors are off at start
}

void MotorControl::applyPwm() {
  if (motorSpeed >= 160) {
    ledcWrite(ENA, motorSpeed);
    ledcWrite(ENB, motorSpeed);
  } else {
    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);
  }
}

void MotorControl::setSpeed(int speed) {
  if (speed < 160) {
    motorSpeed = 0;
    stop();
  } else {
    motorSpeed = constrain(speed, 160, 242);
    applyPwm();
    currentStatus = "Speed set to " + String(motorSpeed);
  }
}

void MotorControl::stop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  motorSpeed = 0;
  applyPwm();
  currentStatus = "Stopped";
}

void MotorControl::forward() {
  applyPwm();
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  currentStatus = "Forward at speed " + String(motorSpeed);
}

void MotorControl::backward() {
  applyPwm();
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  currentStatus = "Backward at speed " + String(motorSpeed);
}

void MotorControl::left() {
  applyPwm();
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  currentStatus = "Left at speed " + String(motorSpeed);
}

void MotorControl::right() {
  applyPwm();
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  currentStatus = "Right at speed " + String(motorSpeed);
}

int MotorControl::getSpeed() {
  return motorSpeed;
}

String MotorControl::getStatus() {
  return currentStatus;
}