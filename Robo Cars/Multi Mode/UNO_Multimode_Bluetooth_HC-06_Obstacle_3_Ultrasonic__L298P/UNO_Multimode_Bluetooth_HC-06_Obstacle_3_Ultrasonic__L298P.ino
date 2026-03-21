#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial BT(2, 3); // RX, TX

const int ENA = 10;
const int IN1 = 12;
const int IN2 = 9;
const int ENB = 11;
const int IN3 = 13;
const int IN4 = 8;

int SPEED; // restored from EEPROM

const int L_TRIG = A5;
const int L_ECHO = A4;
const int C_TRIG = A3;
const int C_ECHO = A2;
const int R_TRIG = A1;
const int R_ECHO = A0;

const int MODE_SWITCH = 7;
bool manualMode = true;

const int BUZZER = 4;

const long DIST_NEAR  = 20;   // base threshold
const long DIST_CLEAR = 40;
const unsigned long ECHO_TIMEOUT = 30000UL;

void setup() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(L_TRIG, OUTPUT); pinMode(L_ECHO, INPUT);
  pinMode(C_TRIG, OUTPUT); pinMode(C_ECHO, INPUT);
  pinMode(R_TRIG, OUTPUT); pinMode(R_ECHO, INPUT);

  pinMode(MODE_SWITCH, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  BT.begin(9600);
  BT.setTimeout(50);
  Serial.begin(9600);

  SPEED = EEPROM.read(0);
  if (SPEED == 0 || SPEED > 255) SPEED = int(115 * 0.95);
  Serial.print("Restored speed from EEPROM: "); Serial.println(SPEED);

  stopCar();
  Serial.println("System initialized. Default mode: BLUETOOTH MANUAL CONTROL");
}

long readDistanceCm(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH, ECHO_TIMEOUT);
  if (duration == 0) return 500;
  return duration / 58;
}

void loop() {
  if (digitalRead(MODE_SWITCH) == LOW) {
    manualMode = !manualMode;
    delay(300);
    stopCar();
    if (manualMode) Serial.println("Mode: BLUETOOTH MANUAL");
    else Serial.println("Mode: OBSTACLE AVOIDANCE");
  }

  if (manualMode) manualControl();
  else obstacleAvoidance();
}

// --- Manual Bluetooth Control ---
void manualControl() {
  while (BT.available()) {
    char first = BT.peek();

    if (first == '#' || isDigit(first)) {
      BT.read();
      String num = BT.readStringUntil('\n'); num.trim();
      int val = num.toInt();
      if (val >= 0 && val <= 255) {
        SPEED = val;
        SPEED = int(SPEED * 1.09);
        if (SPEED > 255) SPEED = 255;
        Serial.print("Speed updated via slider: "); Serial.println(SPEED);

        // Apply new PWM values
        analogWrite(ENA, SPEED);
        analogWrite(ENB, SPEED);

        EEPROM.write(0, SPEED);

        // Clear direction residue: set motor pins neutral
        digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
      }
      continue;
    }

    char cmd = BT.read();

    // Dynamic threshold: base 20 + scaled by speed
    int dynamicThreshold = DIST_NEAR + (SPEED / 10);
    long left   = readDistanceCm(L_TRIG,L_ECHO);
    long center = readDistanceCm(C_TRIG,C_ECHO);
    long right  = readDistanceCm(R_TRIG,R_ECHO);

    if (cmd == 'F') {
      if (left <= dynamicThreshold || center <= dynamicThreshold || right <= dynamicThreshold) {
        stopCar();
        Serial.println("Safety: Wall too close → Forward blocked");
      } else {
        forward(); Serial.println("Manual: Forward");
      }
    } else {
      switch (cmd) {
        case 'B': backward(); Serial.println("Manual: Backward"); break;
        case 'L': strongLeft(); Serial.println("Manual: Strong Left"); break;
        case 'R': strongRight(); Serial.println("Manual: Strong Right"); break;
        case 'S': stopCar(); Serial.println("Manual: Stop"); break;
        case 'T': printDistancesOnce(); break;
      }
    }
  }
}

void printDistancesOnce() {
  Serial.print("L:"); Serial.print(readDistanceCm(L_TRIG,L_ECHO));
  Serial.print(" C:"); Serial.print(readDistanceCm(C_TRIG,C_ECHO));
  Serial.print(" R:"); Serial.println(readDistanceCm(R_TRIG,R_ECHO));
}

// --- Obstacle Avoidance ---
void obstacleAvoidance() {
  long left   = readDistanceCm(L_TRIG,L_ECHO);
  long center = readDistanceCm(C_TRIG,C_ECHO);
  long right  = readDistanceCm(R_TRIG,R_ECHO);

  Serial.print("Distances cm - L:"); Serial.print(left);
  Serial.print(" C:"); Serial.print(center);
  Serial.print(" R:"); Serial.println(right);

  bool LB = (left <= DIST_NEAR);
  bool CB = (center <= DIST_NEAR);
  bool RB = (right <= DIST_NEAR);

  if (LB && CB && RB) {
    Serial.println("Decision: All blocked → 360° turn");
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
    delay(1000);
  }
  else if (CB && LB && !RB) { strongRight(); Serial.println("Decision: Front+Left blocked → Strong Right"); }
  else if (CB && RB && !LB) { strongLeft(); Serial.println("Decision: Front+Right blocked → Strong Left"); }
  else if (CB) { backward(); Serial.println("Decision: Front blocked → Backward"); }
  else if (LB && !CB && !RB) { slightRight(); Serial.println("Decision: Left blocked → Slight Right"); }
  else if (RB && !CB && !LB) { slightLeft(); Serial.println("Decision: Right blocked → Slight Left"); }
  else { forward(); Serial.println("Decision: All clear → Forward"); }

  delay(80);
}

// --- Motor routines ---
void forward() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH); 
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED);
}

void backward() {
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW); 
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH); 
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED);
}

void slightLeft() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH); 
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED / 2);
}

void slightRight() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH); 
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, SPEED / 2);
  analogWrite(ENB, SPEED);
}

void strongLeft() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH); 
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH); 
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED);
}

void strongRight() {
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW); 
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, SPEED); 
  analogWrite(ENB, SPEED);
}

void stopCar() {
  analogWrite(ENA, 0); 
  analogWrite(ENB, 0);
}