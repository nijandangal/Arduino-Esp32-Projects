#include <WiFi.h>
#include <WiFiClient.h>
#include <U8g2lib.h>

// WiFi credentials
const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";

// Remote server (replace with your control server IP/hostname)
const char* host = "192.168.1.100";  // Example: PC running control server
const uint16_t port = 80;

// OLED setup (U8g2)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Pin configuration
#define ENA 25
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ENB 26

const int freq       = 20000;
const int resolution = 8;
const int channelA   = 0;
const int channelB   = 1;

int motorSpeed           = 200;
const int speedThreshold = 160;
String currentStatus     = "Stopped";

// Failsafe watchdog
unsigned long lastCommandTime = 0;
const unsigned long commandTimeout = 5000;

// Runtime limit per command
unsigned long commandStartTime = 0;
const unsigned long maxRunTime = 6000;

WiFiClient client;

void stopCar();

void setup() {
  Serial.begin(115200);

  // OLED init
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"Car Booting...");
  u8g2.sendBuffer();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  ledcAttachChannel(ENA, freq, resolution, channelA);
  ledcAttachChannel(ENB, freq, resolution, channelB);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  stopCar();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"WiFi Connected");
  u8g2.drawStr(0,25,WiFi.localIP().toString().c_str());
  u8g2.sendBuffer();
}

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
    currentStatus = "Speed set to " + String(motorSpeed);
  }
  lastCommandTime = millis();
  Serial.println(currentStatus);

  u8g2.clearBuffer();
  u8g2.drawStr(0,10,currentStatus.c_str());
  u8g2.sendBuffer();
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  currentStatus = "Stopped";
  Serial.println("Stopped");

  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"Stopped");
  u8g2.sendBuffer();
}

void forward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    currentStatus = "Forward at " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
    Serial.println(currentStatus);

    u8g2.clearBuffer();
    u8g2.drawStr(0,10,currentStatus.c_str());
    u8g2.sendBuffer();
  }
}

void backward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    currentStatus = "Backward at " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
    Serial.println(currentStatus);

    u8g2.clearBuffer();
    u8g2.drawStr(0,10,currentStatus.c_str());
    u8g2.sendBuffer();
  }
}

void left() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    currentStatus = "Left at " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
    Serial.println(currentStatus);

    u8g2.clearBuffer();
    u8g2.drawStr(0,10,currentStatus.c_str());
    u8g2.sendBuffer();
  }
}

void right() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    currentStatus = "Right at " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
    Serial.println(currentStatus);

    u8g2.clearBuffer();
    u8g2.drawStr(0,10,currentStatus.c_str());
    u8g2.sendBuffer();
  }
}

void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if      (cmd == "F") forward();
  else if (cmd == "B") backward();
  else if (cmd == "L") left();
  else if (cmd == "R") right();
  else if (cmd == "S") stopCar();
  else if (cmd.startsWith("speed=")) {
    int val = cmd.substring(6).toInt();
    setSpeed(val);
  }
}

// Example: poll remote server for commands
void handleWebClient() {
  if (client.connect(host, port)) {
    client.print(String("GET /cmd HTTP/1.1\r\nHost: ") + host + "\r\nConnection: close\r\n\r\n");
    delay(100);
    String response = client.readString();
    client.stop();

    if (response.indexOf("/F") != -1) forward();
    else if (response.indexOf("/B") != -1) backward();
    else if (response.indexOf("/L") != -1) left();
    else if (response.indexOf("/R") != -1) right();
    else if (response.indexOf("/S") != -1) stopCar();
    else if (response.indexOf("/speed=") != -1) {
      int idx = response.indexOf("/speed=") + 7;
      int val = response.substring(idx).toInt();
      setSpeed(val);
    }
  }
}

void loop() {
  handleSerial();
  handleWebClient();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(100);
  }

  if (millis() - lastCommandTime > commandTimeout) {
    stopCar();
    Serial.println("Failsafe triggered");
    u8g2.clearBuffer();
    u8g2.drawStr(0,10,"Failsafe Stop");
    u8g2.sendBuffer();
  }

  if (currentStatus != "Stopped" && (millis() - commandStartTime > maxRunTime)) {
    stopCar();
    Serial.println("Runtime limit stop");
    u8g2.clearBuffer();
    u8g2.drawStr(0,10,"Runtime Stop");
    u8g2.sendBuffer();
  }
}