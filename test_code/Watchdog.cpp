#include "Watchdog.h"

Watchdog::Watchdog(unsigned long timeout, unsigned long maxRun) {
  commandTimeout = timeout;
  maxRunTime = maxRun;
  lastCommandTime = millis();
  commandStartTime = millis();
}

void Watchdog::resetCommandTimer() {
  lastCommandTime = millis();
}

void Watchdog::resetRunTimer() {
  commandStartTime = millis();
}

void Watchdog::update(MotorControl &motor) {
  if (millis() - lastCommandTime > commandTimeout) {
    motor.stop();
  }
  if (motor.getStatus() != "Stopped" && (millis() - commandStartTime > maxRunTime)) {
    motor.stop();
  }
}