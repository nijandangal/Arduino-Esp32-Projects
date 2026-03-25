#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include "MotorControl.h"

class Watchdog {
  public:
    Watchdog(unsigned long commandTimeout, unsigned long maxRunTime);
    void resetCommandTimer();
    void resetRunTimer();
    void update(MotorControl &motor);

  private:
    unsigned long lastCommandTime;
    unsigned long commandStartTime;
    unsigned long commandTimeout;
    unsigned long maxRunTime;
};

#endif