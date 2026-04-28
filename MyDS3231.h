

#ifndef _MY_DS3231_H_
#define _MY_DS3231_H_
#include <Arduino.h>
#include <Wire.h>

class MyDS3231 {
public:

  bool TimeSts;//時間情報が有効化どうか
  long UECSDate;
  long UECSTime;

  long Year;
  long Mon;
  long Day;
  long Hour;
  long Min;
  long Sec;
  MyDS3231(TwoWire &w);

  bool begin();
  uint8_t toDEC(uint8_t data);
  uint8_t toBCD(int16_t data);
  int getTime();


  uint8_t getAMPM();     //return 0=AM  1=PM
  uint8_t getFormat();   //return 12 or 24 hour format

  void setFormat(uint8_t fmt);  // set time format 12 or 24
  void setAMPM(uint8_t ampm);   // set AM or PM   PM=1 AM=0
  void setTime(int16_t hour,int16_t min,int16_t sec);  
  void setDate(int16_t year,int16_t month,int16_t day); 


private:
  TwoWire *wire;
  const uint8_t _addr= 0x68;
  uint8_t _bin[13];

  uint8_t _minutes;  // 0 - 60 seconds
  uint8_t _seconds;  // 0 - 60 minutes
  uint8_t _hour;     // 1-12 or 0 -23
  uint8_t _ampm;     // 0=AM  1=PM
  uint8_t _timeMode;   // 12 or 24  hour format

  uint8_t _week;    // 1-7  day of week
  uint8_t _date;   // 1-31 date
  uint8_t _month;  // 1- 12 month
  uint8_t _year;   // 0- 99 year

};

#endif
