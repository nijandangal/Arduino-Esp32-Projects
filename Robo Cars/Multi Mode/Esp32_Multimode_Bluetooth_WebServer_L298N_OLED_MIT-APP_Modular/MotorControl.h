#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include <Arduino.h>

class MotorControl {
  public:
    static const int MIN_SPEED = 160;
    static const int MAX_SPEED = 242;

    MotorControl(int ena, int in1, int in2, int in3, int in4, int enb, int freq, int resolution);

    void setSpeed(int targetSpeed);
    void stop();
    void forward();
    void backward();
    void left();
    void right();

    int getSpeed();
    String getStatus();

  private:
    int ENA, IN1, IN2, IN3, IN4, ENB;
    int channelA, channelB;
    int motorSpeed;
    String currentStatus;

    void applyPwm();
};

#endif