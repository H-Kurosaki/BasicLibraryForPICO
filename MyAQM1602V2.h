//AQM1602液晶表示用基本ライブラリ
//ArduinoとRaspberry Pi PICO用

#ifndef MYAQM1602_H
#define MYAQM1602_H

#include <Arduino.h>
#include <Wire.h>
#include "Print.h"

#define AQM1602_I2C_ADDRESS 0x3E 

// commands
#define AQM1602CMD_RETURNHOME 0x02
#define AQM1602CMD_CLEARDISPLAY 0x01
#define AQM1602CMD_DISPLAYCONTROL 0x08
#define AQM1602CMD_DISPLAYON 0x04
#define AQM1602CMD_BLINKON 0x01
#define AQM1602CMD_BLINKOFF 0x00
#define AQM1602CMD_CURSORON 0x02
#define AQM1602CMD_CURSOROFF 0x00
#define AQM1602CMD_CONTRAST 35 //contrast (0-63)

#define LCD_5VMODE	0
#define LCD_3V3MODE	1

class MyAQM1602 : public Print {
  public:
    MyAQM1602(uint8_t addr = AQM1602_I2C_ADDRESS);
    void command(uint8_t value);
    virtual size_t write(uint8_t value);
    void init(void);
    void init(TwoWire &w);
    void init(TwoWire &w,unsigned char voltmode);
    void begin(void);
    void begin(unsigned char voltmode);
    void home(void);
    void clear(void);
    void displayOn(void);
    void newLine(void);
  private:
	TwoWire *wire;
    uint8_t _displayflags;
    uint8_t _i2caddress;
    void send(uint8_t value, uint8_t mode);
};

#endif
