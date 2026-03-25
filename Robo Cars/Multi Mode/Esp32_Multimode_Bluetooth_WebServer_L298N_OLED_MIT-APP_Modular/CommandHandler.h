#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "MotorControl.h"

// Forward declaration of Watchdog (included only in .cpp)
class Watchdog;

class CommandHandler {
  public:
    CommandHandler(MotorControl &motor);
    void begin();   // start WebSocket server
    void loop();    // keep WebSocket alive

  private:
    MotorControl &motor;
    WebSocketsServer webSocket;

    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
};

// declare serveControlPage so test.ino can call it
void serveControlPage(WiFiClient &client, MotorControl &motor);

#endif