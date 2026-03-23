#include <WiFi.h>
#include <WebSocketsClient.h>
#include "MotorControl.h"
#include "OledDisplay.h"
#include "Watchdog.h"

const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";

const char* ws_host = "esp32-car-relay.nijandangal.workers.dev";
const int   ws_port = 443;
const char* ws_path = "/";

MotorControl car(25,16,17,18,19,26,20000,8);
OledDisplay oled;
Watchdog watchdog(5000, 6000);

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

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

  oled.begin();

  webSocket.beginSSL(ws_host, ws_port, ws_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
  watchdog.update(car);

  oled.draw(
    "WiFi Controlled Car",
    "ESP32 WebClient",
    car.getStatus(),
    car.getSpeed(),
    MotorControl::MIN_SPEED,
    MotorControl::MAX_SPEED,
    WiFi.RSSI(),
    WiFi.status() == WL_CONNECTED,
    WiFi.localIP().toString()
  );
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("Connected to Cloudflare Worker WebSocket.");
      webSocket.sendTXT("Status: Connected");
      break;

    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket.");
      car.stop();
      webSocket.sendTXT("Status: Disconnected, car stopped");
      break;

    case WStype_TEXT: {
      String raw = String((char*)payload);
      Serial.println("Raw payload: " + raw);

      watchdog.resetCommandTimer();
      watchdog.resetRunTimer();

      if (raw == "forward") car.forward();
      else if (raw == "backward") car.backward();
      else if (raw == "left") car.left();
      else if (raw == "right") car.right();
      else if (raw.startsWith("speed")) car.setSpeed(raw.substring(5).toInt());
      else if (raw == "stop") car.stop();

      webSocket.sendTXT("Status: " + car.getStatus() + " at speed " + String(car.getSpeed()));
      break;
    }
    default: break;
  }
}