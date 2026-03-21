#include "CommandHandler.h"

CommandHandler::CommandHandler(MotorControl &m) : motor(m) {}

void CommandHandler::handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if      (cmd == "F") motor.forward();
  else if (cmd == "B") motor.backward();
  else if (cmd == "L") motor.left();
  else if (cmd == "R") motor.right();
  else if (cmd == "S") motor.stop();
  else if (cmd.startsWith("speed=")) {
    int val = cmd.substring(6).toInt();
    motor.setSpeed(val);
  }
}

void CommandHandler::handleHttp(WiFiServer &server) {
  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(100);
  String request = client.readStringUntil('\r');
  client.flush();

  if      (request.indexOf("/F") != -1) motor.forward();
  else if (request.indexOf("/B") != -1) motor.backward();
  else if (request.indexOf("/L") != -1) motor.left();
  else if (request.indexOf("/R") != -1) motor.right();
  else if (request.indexOf("/S") != -1) motor.stop();
  else if (request.indexOf("/speed=") != -1) {
    int idx = request.indexOf("/speed=") + 7;
    int val = request.substring(idx).toInt();
    motor.setSpeed(val);
  }

  int barPercent = 0;
  if (motor.getSpeed() >= 160) {
    barPercent = (motor.getSpeed() - 160) * 100 / (242 - 160);
  }

  String html;
  html.reserve(3500);

  // --- Website code unchanged ---
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
  html += "#barFill {height:100%; width:" + String(barPercent) + "%; background:#4CAF50;}";
  html += "</style></head><body>";

  html += "<h2>WiFi Controlled Car with ESP32 and L298 using WebServer Mode</h2>";
  html += "<h3 id='status'>Status: " + motor.getStatus() + "</h3>";
  html += "<div class='grid'>";
  html += "<button class='btn forward' id='btnF'>Forward</button>";
  html += "<button class='btn left' id='btnL'>Left</button>";
  html += "<button class='btn stop' id='btnS'>Stop</button>";
  html += "<button class='btn right' id='btnR'>Right</button>";
  html += "<button class='btn backward' id='btnB'>Backward</button>";
  html += "</div>";

  html += "<div>";
  html += "Speed:<br><input type='range' min='160' max='242' value='" + String(motor.getSpeed()) + "' id='speedSlider'>";
  html += "<span id='val'>" + String(motor.getSpeed()) + "</span>";
  html += "<div id='speedBar'><div id='barFill'></div></div>";
  html += "</div>";

  html += "<script>";
  html += "function updateStatus(msg){document.getElementById('status').innerText='Status: '+msg;}";
  html += "function updateBar(val){var percent=(val-160)*100/82;document.getElementById('barFill').style.width=percent+'%';}";
  html += "function sendCmd(cmd){fetch(cmd,{keepalive:true});}";

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
  html += "  sliderTimer=setTimeout(()=>{sendCmd('/speed='+val);updateStatus('Speed '+val);},150);";
  html += "});";
  html += "updateBar(" + String(motor.getSpeed()) + ");";
  html += "</script>";

  html += "<p>Signal Strength: " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p>Uptime: " + String(millis() / 1000) + " seconds</p>";

  html += "</body></html>";

  client.print(html);
  client.stop();
}