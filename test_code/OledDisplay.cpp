#include "OledDisplay.h"

OledDisplay::OledDisplay() 
  : u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21) {}

void OledDisplay::begin() {
  u8g2.begin();
  u8g2.enableUTF8Print();
  Serial.println("OLED initialized");
}

void OledDisplay::draw(String heading1, String heading2, String status, int speed, int minSpeed, int maxSpeed, int rssi, bool wifiConnected, String ip) {
  u8g2.clearBuffer();
  u8g2.drawFrame(0,0,128,64);

  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawBox(0,0,128,12);
  u8g2.setDrawColor(0);
  int w1 = u8g2.getStrWidth(heading1.c_str());
  u8g2.drawStr((128-w1)/2,10,heading1.c_str());
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_5x8_tr);
  int w2 = u8g2.getStrWidth(heading2.c_str());
  u8g2.drawStr((128-w2)/2,20,heading2.c_str());

  u8g2.drawStr(2,32,("Status: " + status).c_str());
  u8g2.drawStr(2,42,("Speed: " + String(speed)).c_str());

  if (wifiConnected) {
    u8g2.drawStr(2,52,"WiFi: Connected");
    char rssiBuf[20];
    sprintf(rssiBuf,"%d dBm", rssi);
    int wR = u8g2.getStrWidth(rssiBuf);
    u8g2.drawStr(128-wR-2,52,rssiBuf);
    u8g2.drawStr(2,60,("IP: " + ip).c_str());

    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;

    int xStart = 100;
    for (int i=0; i<bars; i++) {
      u8g2.drawBox(xStart + i*4, 60 - i*3, 3, i*3);
    }
  } else {
    u8g2.drawStr(2,52,"WiFi: Not Connected");
  }

  u8g2.sendBuffer();
}