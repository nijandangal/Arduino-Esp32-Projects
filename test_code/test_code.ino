#include <WiFi.h>
#include "MotorControl.h"
#include "CommandHandler.h"
#include "OledDisplay.h"
#include "Watchdog.h"

const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";

WiFiServer server(80);

// Objects
MotorControl car(25,16,17,18,19,26,20000,8);
CommandHandler handler(car);
OledDisplay oled;
Watchdog watchdog(5000, 6000); // 5s timeout, 6s max run

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
  handler.begin();   // start WebSocket server
  oled.begin();
}

void loop() {
  handler.loop();           // process WebSocket first

  WiFiClient client = server.available();
  if (client) {
    serveControlPage(client, car);
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
  }

  watchdog.update(car);

  oled.draw(
    "WiFi Controlled Car",                  // Heading 1
    "ESP32 as WebServer",                   // Heading 2
    car.getStatus(),
    car.getSpeed(),
    MotorControl::MIN_SPEED,
    MotorControl::MAX_SPEED,
    WiFi.RSSI(),
    WiFi.status() == WL_CONNECTED,
    WiFi.localIP().toString()
  );
}