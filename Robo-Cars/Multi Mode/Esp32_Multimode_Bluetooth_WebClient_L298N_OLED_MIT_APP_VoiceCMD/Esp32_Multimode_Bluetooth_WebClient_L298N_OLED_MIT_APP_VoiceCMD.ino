#include <WiFi.h>
#include "BluetoothSerial.h"
#include <U8g2lib.h>

BluetoothSerial SerialBT;

// ===== Pin configuration =====
#define ENA 25
#define IN1 16
#define IN2 17
#define IN3 18
#define IN4 19
#define ENB 26
#define MODE_PIN 33

// ===== PWM setup =====
const int freq       = 20000;
const int resolution = 8;

// ===== Wi-Fi setup =====
const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";
WiFiServer server(80);

// ===== OLED setup =====
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== State variables =====
int motorSpeed           = 200;
const int speedThreshold = 160;
String currentStatus     = "Stopped";

unsigned long lastCommandTime = 0;
const unsigned long commandTimeout = 5000;
unsigned long commandStartTime = 0;
const unsigned long maxRunTime = 6000;

int mode = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastModePrint = 0;

// ===== Animation state =====
int wifiAnimStep = 0;
unsigned long lastWifiAnim = 0;

// ===== OLED update =====
void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);

  // Title
  u8g2.drawStr(30,8,"ESP32 Car");
  u8g2.drawLine(0,10,127,10);

  // Mode with icon + BT connection status
  if(mode==0){
    if(SerialBT.hasClient()){
      u8g2.drawStr(0,20,"BT: Connected");
    } else {
      u8g2.drawStr(0,20,"BT: Not Connected");
    }
    // Bluetooth icon
    u8g2.drawLine(90,15,100,20);
    u8g2.drawLine(100,20,90,25);
  } else {
    u8g2.drawStr(0,20,"Mode: Wi-Fi");

    if(WiFi.status() == WL_CONNECTED){
      u8g2.drawStr(0,30,"Wi-Fi Connected");
      u8g2.drawStr(0,40,(String("IP: ")+WiFi.localIP().toString()).c_str());
      // Wi-Fi arcs
      u8g2.drawCircle(100,20,3,U8G2_DRAW_ALL);
      u8g2.drawCircle(100,20,6,U8G2_DRAW_ALL);
      // Signal bars
      int rssi = WiFi.RSSI();
      int bars = 0;
      if(rssi > -60) bars = 3;
      else if(rssi > -75) bars = 2;
      else if(rssi > -90) bars = 1;
      for(int i=0; i<bars; i++){
        u8g2.drawBox(115 + i*4, 15 - i*3, 3, i*3+3);
      }
    } else {
      // Connecting animation
      if(millis() - lastWifiAnim > 500){
        wifiAnimStep = (wifiAnimStep + 1) % 4;
        lastWifiAnim = millis();
      }
      String dots = "";
      for(int i=0; i<wifiAnimStep; i++) dots += ".";
      u8g2.drawStr(0,30,("Connecting"+dots).c_str());
    }
  }

  // Speed bar
  int barLength = map(motorSpeed, speedThreshold, 242, 0, 100);
  u8g2.drawFrame(0,48,110,8);
  u8g2.drawBox(0,48,barLength,8);
  u8g2.drawStr(0,60,(String("Speed: ")+String(motorSpeed)).c_str());

  // Status
  u8g2.drawStr(0,72,(String("Status: ")+currentStatus).c_str());

  u8g2.sendBuffer();
}

// ===== Motor functions =====
void applyPwm(){ 
  if(motorSpeed>=speedThreshold){ 
    ledcWrite(ENA,motorSpeed); 
    ledcWrite(ENB,motorSpeed);
  } else {
    ledcWrite(ENA,0); 
    ledcWrite(ENB,0);
  }
}
void setSpeed(int speed){ 
  if(speed<speedThreshold){ 
    motorSpeed=0; 
    applyPwm(); 
    stopCar(); 
  } else { 
    motorSpeed=constrain(speed,speedThreshold,242); 
    applyPwm(); 
    currentStatus="Speed "+String(motorSpeed);
  } 
  lastCommandTime=millis(); 
  updateOLED(); 
}
void stopCar(){ 
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW); 
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW); 
  ledcWrite(ENA,0); ledcWrite(ENB,0); 
  currentStatus="Stopped"; 
  updateOLED(); 
}
void forward(){ 
  if(motorSpeed>=speedThreshold){ 
    applyPwm(); 
    digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); 
    digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); 
    currentStatus="Forward"; 
    lastCommandTime=millis(); 
    commandStartTime=millis(); 
    updateOLED(); 
  } 
}
void backward(){ 
  if(motorSpeed>=speedThreshold){ 
    applyPwm(); 
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); 
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); 
    currentStatus="Backward"; 
    lastCommandTime=millis(); 
    commandStartTime=millis(); 
    updateOLED(); 
  } 
}
void left(){ 
  if(motorSpeed>=speedThreshold){ 
    applyPwm(); 
    digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); 
    digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); 
    currentStatus="Left"; 
    lastCommandTime=millis(); 
    commandStartTime=millis(); 
    updateOLED(); 
  } 
}
void right(){ 
  if(motorSpeed>=speedThreshold){ 
    applyPwm(); 
    digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); 
    digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); 
    currentStatus="Right"; 
    lastCommandTime=millis(); 
    commandStartTime=millis(); 
    updateOLED(); 
  } 
}

// ===== Bluetooth handler =====
void handleBluetooth(){
  if(SerialBT.available()){
    char cmd=SerialBT.read();
    lastCommandTime=millis();
    if(cmd=='#'){ 
      String valStr=""; 
      while(SerialBT.available()){ 
        char c=SerialBT.read(); 
        if(isDigit(c)) valStr+=c; else break; 
      } 
      int val=valStr.toInt(); 
      setSpeed(val);
    }
    else{ 
      switch(cmd){ 
        case 'F': forward(); break; 
        case 'B': backward(); break; 
        case 'L': left(); break; 
        case 'R': right(); break; 
        case 'S': stopCar(); break; 
      } 
    }
  }
}

// ===== Wi-Fi handler (webclient style) =====
void handleWebClient(){
  WiFiClient client=server.available();
  if(!client) return;
  client.setTimeout(100);
  String request=client.readStringUntil('\r');
  client.flush();

  if(request.indexOf("/F")!=-1) forward();
  else if(request.indexOf("/B")!=-1) backward();
  else if(request.indexOf("/L")!=-1) left();
  else if(request.indexOf("/R")!=-1) right();
  else if(request.indexOf("/S")!=-1) stopCar();
  else if(request.indexOf("/speed=")!=-1){ 
    int idx=request.indexOf("/speed=")+7; 
    int val=request.substring(idx).toInt(); 
    setSpeed(val); 
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();

  lastCommandTime=millis();
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_RobotCar");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(MODE_PIN, INPUT_PULLUP);

  // PWM setup
  ledcAttach(ENA, freq, resolution);
  ledcAttach(ENB, freq, resolution);

  // OLED init
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(30,8,"ESP32 Car Ready");
  u8g2.drawLine(0,10,127,10);
  u8g2.sendBuffer();

  stopCar();
  motorSpeed = 0;

  // Wi-Fi init
  WiFi.begin(ssid, password);
  server.begin();

  mode = 0;
  Serial.println("System ready. Default mode: Bluetooth");
  updateOLED();
}

// ===== Loop =====
void loop() {
  int buttonState = digitalRead(MODE_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    lastDebounceTime = millis();
  }

  if (buttonState == LOW && (millis() - lastDebounceTime) > 300) {
    mode = 1 - mode;
    if (mode == 0) {
      Serial.println("Switched to Bluetooth mode");
      SerialBT.println("Switched to Bluetooth mode");
    } else {
      Serial.println("Switched to Wi-Fi mode");
    }
    updateOLED();
    while (digitalRead(MODE_PIN) == LOW) {
      delay(10);
    }
    lastDebounceTime = millis();
  }

  lastButtonState = buttonState;

  if (mode == 0) {
    handleBluetooth();
  } else {
    handleWebClient();   // swapped from handleHttp()
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

  if (millis() - lastModePrint > 2000) {
    if (mode == 0) {
      Serial.println("Mode: Bluetooth");
    } else {
      Serial.println("Mode: Wi-Fi");
    }
    updateOLED();
    lastModePrint = millis();
  }
}