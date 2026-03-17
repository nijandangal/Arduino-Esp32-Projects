#include <WiFi.h>
#include "BluetoothSerial.h"
#include <U8g2lib.h>

BluetoothSerial SerialBT;

// Pin configuration
#define ENA 25
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ENB 26
#define MODE_PIN 33

// PWM setup
const int freq = 20000;
const int resolution = 8;

// Wi-Fi setup
const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";
WiFiServer server(80);

// OLED setup
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// State variables
int motorSpeed = 200;
const int speedThreshold = 160;
String currentStatus = "Stopped";

unsigned long lastCommandTime = 0;
const unsigned long commandTimeout = 5000;
unsigned long commandStartTime = 0;
const unsigned long maxRunTime = 6000;

int mode = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

// ===== Motor functions =====
void applyPwm() {
  if (motorSpeed >= speedThreshold) {
    ledcWrite(ENA, motorSpeed);
    ledcWrite(ENB, motorSpeed);
  } else {
    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);
  }
}

void setSpeed(int speed) {
  if (speed < speedThreshold) {
    motorSpeed = 0;
    applyPwm();
    stopCar();
  } else {
    motorSpeed = constrain(speed, speedThreshold, 242);
    applyPwm();
    currentStatus = "Speed " + String(motorSpeed);
  }
  lastCommandTime = millis();
}

void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  currentStatus = "Stopped";
}

void forward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    currentStatus = "Forward";
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void backward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    currentStatus = "Backward";
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void left() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    currentStatus = "Left";
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void right() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    currentStatus = "Right";
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

// ===== Wi-Fi handler =====
void handleHttp() {
  WiFiClient client = server.available();
  if (!client) return;
  client.setTimeout(100);
  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/F") != -1) forward();
  else if (request.indexOf("/B") != -1) backward();
  else if (request.indexOf("/L") != -1) left();
  else if (request.indexOf("/R") != -1) right();
  else if (request.indexOf("/S") != -1) stopCar();
  else if (request.indexOf("/speed=") != -1) {
    int idx = request.indexOf("/speed=") + 7;
    int val = request.substring(idx).toInt();
    setSpeed(val);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"status\":\"" + currentStatus + "\"}");
  client.stop();
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_RobotCar");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(MODE_PIN, INPUT_PULLUP);

  ledcAttach(ENA, freq, resolution);
  ledcAttach(ENB, freq, resolution);

  WiFi.begin(ssid, password);
  server.begin();

  stopCar();
  motorSpeed = 0;

  Serial.println("System ready. Default mode: Bluetooth");
}

// ===== Loop =====
void loop() {
  if (mode == 0) {
    // Bluetooth handler here if needed
  } else {
    handleHttp();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }

  if (millis() - lastCommandTime > commandTimeout) {
    stopCar();
  }

  if (currentStatus != "Stopped" && (millis() - commandStartTime > maxRunTime)) {
    stopCar();
  }
}