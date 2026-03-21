#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include "MotorControl.h"

class CommandHandler {
  public:
    CommandHandler(MotorControl &motor);
    void handleSerial();
    void handleHttp(WiFiServer &server);

  private:
    MotorControl &motor;
};

#endif