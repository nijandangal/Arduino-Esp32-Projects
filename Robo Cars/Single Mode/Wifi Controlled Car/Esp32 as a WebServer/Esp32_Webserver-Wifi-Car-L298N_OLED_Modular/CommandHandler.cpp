#include "CommandHandler.h"
#include "Watchdog.h"

extern Watchdog watchdog;

CommandHandler::CommandHandler(MotorControl &m) 
  : motor(m), webSocket(81) {}

void CommandHandler::begin() {
  webSocket.begin();
  webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    this->onWebSocketEvent(num, type, payload, length);
  });
  Serial.println("[WebSocket] Server started on port 81");
}

void CommandHandler::loop() {
  webSocket.loop();
}

void CommandHandler::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String cmd = String((char*)payload);
    cmd.trim();
    Serial.println("[WebSocket] Client " + String(num) + " sent: " + cmd);

    if      (cmd == "F") { motor.forward(); watchdog.resetCommandTimer(); watchdog.resetRunTimer(); }
    else if (cmd == "B") { motor.backward(); watchdog.resetCommandTimer(); watchdog.resetRunTimer(); }
    else if (cmd == "L") { motor.left(); watchdog.resetCommandTimer(); watchdog.resetRunTimer(); }
    else if (cmd == "R") { motor.right(); watchdog.resetCommandTimer(); watchdog.resetRunTimer(); }
    else if (cmd == "S") { motor.stop(); watchdog.resetCommandTimer(); }
    else if (cmd.startsWith("speed=")) {
      int val = cmd.substring(6).toInt();
      motor.setSpeed(val);
      watchdog.resetCommandTimer();
    }

    String statusMsg = "Status:" + motor.getStatus();
    webSocket.sendTXT(num, statusMsg);
    Serial.println("[WebSocket] Sent back: " + statusMsg);
  }
}

void serveControlPage(WiFiClient &client, MotorControl &motor) {
  if (client.available()) {
    String req = client.readStringUntil('\r');
    if (req.indexOf("favicon.ico") != -1) {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Connection: close");
      client.println();
      client.stop();
      return;
    }
  }

  int barPercent = (motor.getSpeed() - MotorControl::MIN_SPEED) * 100 / (MotorControl::MAX_SPEED - MotorControl::MIN_SPEED);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<style>");
  client.println("body {font-family: Arial; text-align:center; margin:0; padding:0;}");
  client.println(".grid {display:grid; grid-template-columns: 1fr 1fr 1fr; grid-template-rows: auto auto auto; gap:10px;}");
  client.println(".btn {width:100%; height:80px; font-size:20px; border:none; border-radius:10px; user-select:none; touch-action:none;}");
  client.println(".forward {background:#4CAF50; color:white; grid-column:2; grid-row:1;}");
  client.println(".backward {background:#f44336; color:white; grid-column:2; grid-row:3;}");
  client.println(".left {background:#2196F3; color:white; grid-column:1; grid-row:2;}");
  client.println(".right {background:#FF9800; color:white; grid-column:3; grid-row:2;}");
  client.println(".stop {background:#555; color:white; grid-column:2; grid-row:2;}");
  client.println("#speedBar {width:90%; height:20px; background:#ddd; margin:10px auto; border-radius:10px; overflow:hidden;}");
  client.println("#barFill {height:100%; width:" + String(barPercent) + "%; background:#4CAF50;}");
  client.println("</style></head><body>");

  client.println("<h2>WiFi Controlled Car</h2>");
  client.println("<h3>ESP32 as WebServer</h3>");
  client.println("<h3 id='status'>Status: " + motor.getStatus() + "</h3>");
  client.println("<div class='grid'>");
  client.println("<button class='btn forward' id='btnF'>Forward</button>");
  client.println("<button class='btn left' id='btnL'>Left</button>");
  client.println("<button class='btn stop' id='btnS'>Stop</button>");
  client.println("<button class='btn right' id='btnR'>Right</button>");
  client.println("<button class='btn backward' id='btnB'>Backward</button>");
  client.println("</div>");

  client.println("<div>");
  client.println("Speed:<br><input type='range' min='" + String(MotorControl::MIN_SPEED) + "' max='" + String(MotorControl::MAX_SPEED) + "' step='1' value='" + String(motor.getSpeed()) + "' id='speedSlider'>");
  client.println("<span id='val'>" + String(motor.getSpeed()) + "</span>");
  client.println("<div id='speedBar'><div id='barFill'></div></div>");
  client.println("</div>");

  client.println("<script>");
  client.println("var ws = new WebSocket('ws://' + location.hostname + ':81/');");
  client.println("ws.onmessage = function(event){document.getElementById('status').innerText=event.data;};");
  client.println("function sendCmd(cmd){ws.send(cmd);}");
  client.println("function bindButton(id, cmd){");
  client.println(" var btn=document.getElementById(id);");
  client.println(" btn.addEventListener('mousedown',()=>sendCmd(cmd));");
  client.println(" btn.addEventListener('mouseup',()=>sendCmd('S'));");
  client.println(" btn.addEventListener('mouseleave',()=>sendCmd('S'));");
  client.println(" btn.addEventListener('touchstart',(e)=>{e.preventDefault();sendCmd(cmd);});");
  client.println(" btn.addEventListener('touchend',(e)=>{e.preventDefault();sendCmd('S');});");
  client.println(" btn.addEventListener('touchcancel',(e)=>{e.preventDefault();sendCmd('S');});");
  client.println("}");
  client.println("bindButton('btnF','F');");
  client.println("bindButton('btnB','B');");
  client.println("bindButton('btnL','L');");
  client.println("bindButton('btnR','R');");
  client.println("document.getElementById('btnS').addEventListener('click',()=>sendCmd('S'));");
  client.println("var slider=document.getElementById('speedSlider');");
  client.println("slider.addEventListener('input',function(){");
  client.println(" var val=parseInt(this.value);");
  client.println(" document.getElementById('val').innerText=val;");
  client.println(" sendCmd('speed='+val);");
  client.println("});");
  client.println("</script>");

  client.println("<p>Signal Strength: " + String(WiFi.RSSI()) + " dBm</p>");
  client.println("<p>Uptime: " + String(millis() / 1000) + " seconds</p>");
  client.println("</body></html>");

  delay(1);
  client.flush();
  client.stop();
}