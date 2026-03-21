#include <SoftwareSerial.h>

// HC-06 Bluetooth module on D2 (RX), D3 (TX)
SoftwareSerial BT(2, 3); // RX, TX

// Motor driver pins (Fundumoto L298P Shield)
const int ENA = 10;
const int IN1 = 12;
const int IN2 = 9;

const int ENB = 11;
const int IN3 = 13;
const int IN4 = 8;

int SPEED = 115; // default speed

// IR sensors
const int IR_LEFT   = A0;
const int IR_CENTER = A1;
const int IR_RIGHT  = A2;

// Mode toggle switch (momentary)
const int MODE_SWITCH = 7;  
bool manualMode = true;    // start in Bluetooth manual mode by default

// Buzzer
const int BUZZER = 4;

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(IR_LEFT, INPUT);
  pinMode(IR_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);

  pinMode(MODE_SWITCH, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  BT.begin(9600); // HC-06 default baud rate
  Serial.begin(9600);

  stopCar();
  Serial.println("System initialized.");
  Serial.println("Default mode: BLUETOOTH MANUAL CONTROL");
}

void loop() {
  // --- Mode toggle ---
  if (digitalRead(MODE_SWITCH) == LOW) {
    manualMode = !manualMode;   // toggle mode
    delay(300);                 // debounce
    stopCar();
    if (manualMode) {
      singleBeep();
      Serial.println("Mode switched: BLUETOOTH MANUAL CONTROL");
    } else {
      doubleBeep();
      Serial.println("Mode switched: OBSTACLE AVOIDANCE");
    }
  }

  if (manualMode) {
    manualControl();
  } else {
    obstacleAvoidance();
  }
}

// --- Manual Bluetooth Control ---
void manualControl() {
  if (BT.available()) {
    char cmd = BT.read();
    Serial.print("Bluetooth command received: ");
    Serial.println(cmd);

    switch (cmd) {
      case 'F': forward(); Serial.println("Action: Forward"); break;
      case 'B': backward(); Serial.println("Action: Backward"); break;
      case 'L': strongLeft(); Serial.println("Action: Strong Left"); break;
      case 'R': strongRight(); Serial.println("Action: Strong Right"); break;
      case 'S': stopCar(); Serial.println("Action: Stop"); break;
      case '#': { // speed slider
        int val = BT.parseInt();
        if (val >= 0 && val <= 255) {
          SPEED = val;
          Serial.print("Speed updated via slider: ");
          Serial.println(SPEED);
        }
        break;
      }
    }
  }
}

// --- Autonomous Obstacle Avoidance ---
void obstacleAvoidance() {
  int left   = digitalRead(IR_LEFT);
  int center = digitalRead(IR_CENTER);
  int right  = digitalRead(IR_RIGHT);

  Serial.print("Sensors - Left: "); Serial.print(left);
  Serial.print(" Center: "); Serial.print(center);
  Serial.print(" Right: "); Serial.println(right);

  if (left == LOW && center == LOW && right == LOW) {
    longBeep();
    backward(); delay(310); stopCar();
    Serial.println("Decision: All blocked → Backward + Stop");
  }
  else if (center == LOW && left == LOW && right == HIGH) {
    doubleBeep();
    strongRight(); delay(440); stopCar();
    Serial.println("Decision: Front+Left blocked → Strong Right");
  }
  else if (center == LOW && right == LOW && left == HIGH) {
    doubleBeep();
    strongLeft(); delay(440); stopCar();
    Serial.println("Decision: Front+Right blocked → Strong Left");
  }
  else if (center == LOW) {
    longBeep();
    backward(); delay(310); stopCar();
    Serial.println("Decision: Front blocked → Backward + Stop");
  }
  else if (left == LOW && right == HIGH && center == HIGH) {
    singleBeep();
    slightRight(); delay(250); stopCar();
    Serial.println("Decision: Left blocked → Slight Right");
  }
  else if (right == LOW && left == HIGH && center == HIGH) {
    singleBeep();
    slightLeft(); delay(250); stopCar();
    Serial.println("Decision: Right blocked → Slight Left");
  }
  else {
    if (left == HIGH && center == HIGH && right == HIGH) {
      forward();
      Serial.println("Decision: Path clear → Forward");
    } else {
      stopCar();
      Serial.println("Decision: Stop");
    }
  }
}

// --- Motor routines ---
void forward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
}

void backward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
}

void slightLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED / 2);  
  analogWrite(ENB, SPEED);
}

void slightRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED);
  analogWrite(ENB, SPEED / 2);  
}

void strongLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
}

void strongRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
}

void stopCar() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// --- Buzzer routines ---
void singleBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}

void doubleBeep() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}

void longBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(300);
  digitalWrite(BUZZER, LOW);
}