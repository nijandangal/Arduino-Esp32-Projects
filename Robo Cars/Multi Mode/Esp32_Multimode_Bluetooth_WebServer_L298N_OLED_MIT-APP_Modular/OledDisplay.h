#ifndef OLEDDISPLAY_H
#define OLEDDISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>

class OledDisplay {
  public:
    OledDisplay();
    void begin();
    void draw(String heading1, String heading2, String status, int speed, int minSpeed, int maxSpeed, int rssi, bool wifiConnected, String ip);

  private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
};

#endif