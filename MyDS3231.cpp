/*
  This Library is written for DS3231 RTC
  Author: Bonezegei (Jofel Batutay)
  Date: Jan 26 2024
*/

#include "MyDS3231.h"


MyDS3231::MyDS3231(TwoWire &w) {
	wire = &w;
}

bool MyDS3231::begin() {

  TimeSts=false;
  return 1;
}

uint8_t MyDS3231::toDEC(uint8_t data) {
  uint8_t result = data & 0x0f;
  result += ((data & 0xf0) >> 4) * 10;

  
  return result;
}

uint8_t MyDS3231::toBCD(int16_t data) {
  int16_t tmp1;
  if (data > 10) {
    tmp1 = (int16_t)(data / 10);
  } else {
    tmp1 = 0;
  }
  int16_t tmp2 = data - (tmp1 * 10);

  uint8_t result = (tmp1 << 4) | tmp2;


  return result;
}

int MyDS3231::getTime() {
  wire->beginTransmission(_addr);
  wire->write(0x00);
  wire->endTransmission();delay(1);

  int16_t a = 0;
  int16_t r = (uint8_t)wire->requestFrom((int16_t)_addr, 13);
  while (wire->available()) {
    _bin[a] = wire->read();
//    Serial.print(_bin[a],HEX);
    a++;
  }

  //time
  _seconds = toDEC(_bin[0]);  //0 - 60 seconds
  _minutes = toDEC(_bin[1]);  //0 - 60 minutes

  if (_bin[2] & 0x40) {
    _hour = toDEC(_bin[2] & 0x1F);
    _ampm = (_bin[2] & 0x20) >> 5;
    _timeMode = 12;
  } else {
    _hour = toDEC(_bin[2] & 0x3F);
    _timeMode = 24;
  }

  //date
  _week = _bin[3];                    // 1-7  day of week
  _date = toDEC(_bin[4]);          // 1-31 date
  _month = toDEC(_bin[5] & 0x3f);  // 1- 12 month
  _year = toDEC(_bin[6]);          // 0- 99 year

  if(r && _year>20 && _date>0 && _date<32 && _month>0 && _month<13)
    {
      TimeSts=true;
      UECSDate=(long)_year*10000+(long)_month*100+(long)_date;
      UECSTime=(long)_hour*10000+(long)_minutes*100+(long)_seconds;
    Year=2000+(long)_year;
    Mon=_month;
    Day=_date;
    Hour=_hour;
    Min=_minutes;
    Sec=_seconds;            
    }
  else
    {
      TimeSts=false;
    }

  return r;
}


uint8_t MyDS3231::getAMPM() {
  return _ampm;
}

uint8_t MyDS3231::getFormat() {
  return _timeMode;
}

void MyDS3231::setFormat(uint8_t fmt) {
  getTime();

  if (fmt == 12) {
    _bin[2] |= 0x40;
    wire->beginTransmission(_addr);
    wire->write(0x02);
    wire->write(_bin[2]);
    wire->endTransmission();delay(1);
    _timeMode = 12;
  } else {
    _bin[2] &= 0x3f;
    wire->beginTransmission(_addr);
    wire->write(0x02);
    wire->write(_bin[2]);
    wire->endTransmission();delay(1);
    _timeMode = 24;
  }
}

void MyDS3231::setAMPM(uint8_t ampm) {
  getTime();

  if (ampm == 1) {
    _bin[2] |= 0x20;
    wire->beginTransmission(_addr);
    wire->write(0x02);
    wire->write(_bin[2]);
    wire->endTransmission();delay(1);
    _ampm = 1;
  } else {
    _bin[2] &= 0x5f;
    wire->beginTransmission(_addr);
    wire->write(0x02);
    wire->write(_bin[2]);
    wire->endTransmission();delay(1);
    _ampm = 0;
  }
}

void MyDS3231::setTime(int16_t hour,int16_t min,int16_t sec) {

  _bin[0] = toBCD(sec);
  _bin[1] = toBCD(min);
  _bin[2] &= 0x60;
  _bin[2] = toBCD(hour);

  wire->beginTransmission(_addr);
  wire->write(0x00);
  wire->write(_bin[0]);
  wire->write(_bin[1]);
  wire->write(_bin[2]);
  wire->endTransmission();delay(1);
}

void MyDS3231::setDate(int16_t year,int16_t month,int16_t day) {

  if(year>2000)
    {year=year-2000;}

  _bin[4] = toBCD(day);
  _bin[5] &= 0x80;
  _bin[5] = toBCD(month);
  _bin[6] = toBCD(year);

  wire->beginTransmission(_addr);
  wire->write(0x04);
  wire->write(_bin[4]);
  wire->write(_bin[5]);
  wire->write(_bin[6]);
  wire->endTransmission();delay(1);
}
