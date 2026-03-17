#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

// Motor pins
#define ENA 25   // ✅ Remapped to PWM-friendly pin
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ENB 26   // ✅ Remapped to PWM-friendly pin

// Speed settings
int motorSpeed = 180;            // Default speed
const int minSpeed = 100;        // ✅ Lower minimum speed
const int maxSpeed = 230;        // ✅ Cap top speed below full 255
char lastCommand = 'S';          // Track last command

// Failsafe
unsigned long lastCommandTime = 0;     
const unsigned long timeout = 2000;    // 2 seconds failsafe

// ✅ LEDC PWM setup (ESP32 v3.3.7 API)
const int freq = 5000;   // 5 kHz for quieter, smoother motor control
const int resolution = 8;    // 8-bit resolution (0–255)

// Motor functions
void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  Serial.println("Stopped");
  SerialBT.println("Stopped");   // ✅ Feedback to app
}

void forward() {
  if (motorSpeed >= minSpeed) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    ledcWrite(ENA, motorSpeed); ledcWrite(ENB, motorSpeed);
    Serial.println("Forward");
    SerialBT.println("Forward"); // ✅ Feedback to app
  }
}

void backward() {
  if (motorSpeed >= minSpeed) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    ledcWrite(ENA, motorSpeed); ledcWrite(ENB, motorSpeed);
    Serial.println("Backward");
    SerialBT.println("Backward"); // ✅ Feedback to app
  }
}

void left() {
  if (motorSpeed >= minSpeed) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    ledcWrite(ENA, motorSpeed); ledcWrite(ENB, motorSpeed);
    Serial.println("Left");
    SerialBT.println("Left"); // ✅ Feedback to app
  }
}

void right() {
  if (motorSpeed >= minSpeed) {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    ledcWrite(ENA, motorSpeed); ledcWrite(ENB, motorSpeed);
    Serial.println("Right");
    SerialBT.println("Right"); // ✅ Feedback to app
  }
}

void setSpeed(int speed) {
  if (speed < minSpeed) {
    motorSpeed = 0;
    stopCar();
  } else {
    motorSpeed = constrain(speed, minSpeed, maxSpeed);
    ledcWrite(ENA, motorSpeed);
    ledcWrite(ENB, motorSpeed);
    Serial.print("Speed set to: ");
    Serial.println(motorSpeed);
    SerialBT.print("Speed set to: ");  // ✅ Feedback to app
    SerialBT.println(motorSpeed);
  }
}

void setup() {
  Serial.begin(9600); // ✅ Match Serial Monitor baud rate
  SerialBT.begin("ESP32_RobotCar"); // Bluetooth name

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // ✅ Attach PWM pins directly (ESP32 v3.3.7 API)
  ledcAttach(ENA, freq, resolution);
  ledcAttach(ENB, freq, resolution);

  stopCar();
  Serial.println("Bluetooth started. Ready to connect!");
  SerialBT.println("Bluetooth started. Ready to connect!"); // ✅ Feedback to app
}

void loop() {
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    lastCommandTime = millis();        // ✅ Reset timer whenever a command arrives

    if (cmd == '#') {
      String valStr = "";
      while (SerialBT.available()) {
        char c = SerialBT.read();
        if (isDigit(c)) valStr += c;
        else break;
      }
      int val = valStr.toInt();
      setSpeed(val);
    } else {
      if (cmd != lastCommand) {
        lastCommand = cmd;
        switch (cmd) {
          case 'F': forward(); break;
          case 'B': backward(); break;
          case 'L': left(); break;
          case 'R': right(); break;
          case 'S': stopCar(); break;
        }
      }
    }
  }

  // ✅ Failsafe check
  if (millis() - lastCommandTime > timeout && lastCommand != 'S') {
    stopCar();
    lastCommand = 'S';   // Prevent repeated stop calls
    Serial.println("Failsafe stop triggered");
    SerialBT.println("Failsafe stop triggered"); // ✅ Feedback to app
  }
}