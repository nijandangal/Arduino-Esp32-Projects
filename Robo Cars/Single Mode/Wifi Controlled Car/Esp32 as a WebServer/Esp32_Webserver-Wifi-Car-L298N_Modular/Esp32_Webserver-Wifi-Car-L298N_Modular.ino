#include <WiFi.h>
#include "MotorControl.h"
#include "CommandHandler.h"

const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";

WiFiServer server(80);

// Motor + Command handler objects
MotorControl car(25,16,17,18,19,26,20000,8);
CommandHandler handler(car);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  handler.handleSerial();
  handler.handleHttp(server);

  // WiFi reconnect logic
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
  }
}