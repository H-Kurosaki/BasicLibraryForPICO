
#ifndef _MYDS18B20_H_
#define _MYDS18B20_H_

#include "Arduino.h"
#include "_microDS18B20.h"//https://github.com/GyverLibs/microDS18B20


#define DS18B20_MAXSENSORS    2
#define DS18B20_SENSOR1   0
#define DS18B20_SENSOR2   1



//センサの状態を保持する構造体
struct ds18sts{
//  byte addr[8]; //センサ固有アドレス
//  byte type;    //センサ分解能
  bool enabled; //接続確認済みフラグ
  byte measurecount;//連続計測成功数(max10)
  byte errorcount;//連続計測失敗数(max10)
  double TempTemp;//直前の値を保存
  unsigned char count;//コマンドの与えた状態を保存
};

class DS18B20 {
  public:
    ds18sts dssts[DS18B20_MAXSENSORS];
    double autoRead1Sec(unsigned char ch);
    private:	
	//MicroDS18B20<26> sensor0;//GP26(ADC0)
  MicroDS18B20<28> sensor0;//GP28(ADC2)
};

#endif
