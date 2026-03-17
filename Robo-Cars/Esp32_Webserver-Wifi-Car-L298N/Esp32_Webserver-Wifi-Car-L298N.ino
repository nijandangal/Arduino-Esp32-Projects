#include <WiFi.h>

const char* ssid     = "nijandangal_2.4";
const char* password = "_4iCZDQQJ0O7C7";

WiFiServer server(80);

// Pin configuration (PWM-friendly ENA/ENB)
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
const unsigned long commandTimeout = 5000; // 2 seconds

// Runtime limit per command
unsigned long commandStartTime = 0;
const unsigned long maxRunTime = 6000; // 5 seconds

void stopCar();

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // ESP32 core 3.x LEDC API
  ledcAttachChannel(ENA, freq, resolution, channelA);
  ledcAttachChannel(ENB, freq, resolution, channelB);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  stopCar();

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
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  currentStatus = "Stopped";
}

void forward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    currentStatus = "Forward at speed " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void backward() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    currentStatus = "Backward at speed " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void left() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    currentStatus = "Left at speed " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
  }
}

void right() {
  if (motorSpeed >= speedThreshold) {
    applyPwm();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    currentStatus = "Right at speed " + String(motorSpeed);
    lastCommandTime = millis();
    commandStartTime = millis();
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

void handleHttp() {
  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(100);
  String request = client.readStringUntil('\r');
  client.flush();

  if      (request.indexOf("/F") != -1) forward();
  else if (request.indexOf("/B") != -1) backward();
  else if (request.indexOf("/L") != -1) left();
  else if (request.indexOf("/R") != -1) right();
  else if (request.indexOf("/S") != -1) stopCar();
  else if (request.indexOf("/speed=") != -1) {
    int idx = request.indexOf("/speed=") + 7;
    int val = request.substring(idx).toInt();
    setSpeed(val);
  }

  int barPercent = 0;
  if (motorSpeed >= speedThreshold) {
    barPercent = (motorSpeed - speedThreshold) * 100 / (242 - speedThreshold);
  }

  String html;
  html.reserve(3500);

  html += "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body {font-family: Arial; text-align:center; margin:0; padding:0;}";
  html += ".grid {display:grid; grid-template-columns: 1fr 1fr 1fr; grid-template-rows: auto auto auto; gap:10px;}";
  html += ".btn {width:100%; height:80px; font-size:20px; border:none; border-radius:10px;";
  html += "user-select:none; touch-action:none; -webkit-user-select:none; -webkit-touch-callout:none;}";
  html += ".forward {background:#4CAF50; color:white; grid-column:2; grid-row:1;}";
  html += ".backward {background:#f44336; color:white; grid-column:2; grid-row:3;}";
  html += ".left {background:#2196F3; color:white; grid-column:1; grid-row:2;}";
  html += ".right {background:#FF9800; color:white; grid-column:3; grid-row:2;}";
  html += ".stop {background:#555; color:white; grid-column:2; grid-row:2;}";
  html += "#speedBar {width:90%; height:20px; background:#ddd; margin:10px auto; border-radius:10px; overflow:hidden;}";
  html += "#barFill {height:100%; width:";
  html += String(barPercent);
  html += "%; background:#4CAF50;}";
  html += "</style></head><body>";

  html += "<h2>WiFi Controlled Car with ESP32 and L298 using WebServer Mode</h2>";
  html += "<h3 id='status'>Status: " + currentStatus + "</h3>";
    html += "<div class='grid'>";
  html += "<button class='btn forward' id='btnF'>Forward</button>";
  html += "<button class='btn left' id='btnL'>Left</button>";
  html += "<button class='btn stop' id='btnS'>Stop</button>";
  html += "<button class='btn right' id='btnR'>Right</button>";
  html += "<button class='btn backward' id='btnB'>Backward</button>";
  html += "</div>";

  html += "<div>";
  html += "Speed:<br><input type='range' min='160' max='242' value='";
  html += String(motorSpeed);
  html += "' id='speedSlider'>";
  html += "<span id='val'>";
  html += String(motorSpeed);
  html += "</span>";
   html += "<div id='speedBar'><div id='barFill'></div></div>";
  html += "</div>";

  html += "<script>";
  html += "function updateStatus(msg){document.getElementById('status').innerText='Status: '+msg;}";
  html += "function updateBar(val){var percent=(val-160)*100/82;document.getElementById('barFill').style.width=percent+'%';}";
  html += "function sendCmd(cmd){fetch(cmd,{keepalive:true});}";

  // Press = start, Release = stop
  html += "function bindButton(id, cmd,label){";
  html += "  var btn=document.getElementById(id);";
  html += "  btn.addEventListener('touchstart',function(e){e.preventDefault();sendCmd(cmd);updateStatus(label);});";
  html += "  btn.addEventListener('touchend',function(e){e.preventDefault();sendCmd('/S');updateStatus('Stopped');});";
  html += "  btn.addEventListener('touchcancel',function(e){e.preventDefault();sendCmd('/S');updateStatus('Stopped');});";
  html += "  btn.addEventListener('mousedown',function(e){e.preventDefault();sendCmd(cmd);updateStatus(label);});";
  html += "  btn.addEventListener('mouseup',function(e){e.preventDefault();sendCmd('/S');updateStatus('Stopped');});";
  html += "  btn.addEventListener('mouseleave',function(e){sendCmd('/S');updateStatus('Stopped');});";
  html += "}";

  html += "bindButton('btnF','/F','Forward');";
  html += "bindButton('btnB','/B','Backward');";
  html += "bindButton('btnL','/L','Left');";
  html += "bindButton('btnR','/R','Right');";
  html += "document.getElementById('btnS').addEventListener('click',function(e){e.preventDefault();sendCmd('/S');updateStatus('Stopped');});";

  html += "let sliderTimer=null;";
  html += "var slider=document.getElementById('speedSlider');";
  html += "slider.addEventListener('input',function(){";
  html += "  clearTimeout(sliderTimer);";
  html += "  var val=this.value;";
  html += "  document.getElementById('val').innerText=val;";
  html += "  updateBar(val);";
  html += "  sliderTimer=setTimeout(()=>{sendCmd('/speed='+val);updateStatus('Speed '+val);},150);"; // safer debounce
  html += "});";
  html += "updateBar(" + String(motorSpeed) + ");";
  html += "</script>";

  // Extra status info: WiFi RSSI and uptime
  html += "<p>Signal Strength: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p>Uptime: " + String(millis() / 1000) + " seconds</p>";

  html += "</body></html>";

  client.print(html);
  client.stop();
}

void loop() {
  handleSerial();
  handleHttp();

  // WiFi reconnect logic
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(100);
  }

  // Failsafe auto-stop if no command received recently
  if (millis() - lastCommandTime > commandTimeout) {
    stopCar();
  }

  // Runtime limit per command (auto-stop after 5 seconds continuous run)
  if (currentStatus != "Stopped" && (millis() - commandStartTime > maxRunTime)) {
    stopCar();
  }
}