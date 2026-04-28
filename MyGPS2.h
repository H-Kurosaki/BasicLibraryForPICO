//MyGPS Library
//秋月電子AE-GYSFDMAXB GPSモジュール用ライブラリ
//Arduino MEGA,Raspberry Pi PICO(バッファ拡張版)
//開発：H.Kurosaki
//Ver2.1 2026/4/8
//このライブラリはArduno標準ライブラリがあれば単体でも使用できる
//注意:日本国内でしか使用できません(標準時が日本で北半球に固定されているため)
/*
対応モジュール
秋月電子 AE-GYSFDMAXB(ディスコン)
秋月電子 AE-GNSS-EXTANT(注意:AE-GYSFDMAXBとRX,TXピンが逆)
GT-902PMGG(注意:日付情報が3分程度しか保持されない)
*/

/*
ライセンス
Copyright 2024 Hideto Kurosaki

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef _GPS_H_
#define _GPS_H_

#include "Arduino.h"

#define GPS_SERIAL_NG  -1  
#define GPS_SERIAL_OTHER 0
#define GPS_SERIAL_ZDA 1
#define GPS_SERIAL_GSV 2
#define GPS_SERIAL_GLL 3
#define GPS_SERIAL_RMC 4
	
#define JST 9 //Japan Standard Time

#define SERIAL_BUFFSIZE 160

#define PAI 3.14159265359
#define TO_RAD 0.01745329251


class GPS {
  public:
bool RMCSts;//RMC受信可能か
bool GPSSts;//GPSが使えるか
bool TimeSts;//時間情報が有効化どうか
bool PositionSts;//位置情報が有効かどうか

//現在の日付時刻
long UECSDate;
long UECSTime;
long Year;
long Mon;
long Day;
long Hour;
long Min;
long Sec;

double HourOfTheToday;//今日が始まってからの経過時間
long DayOfTheYear;//元旦からの経過日数
double SolarAltDeg;
double SolarAziDeg;
double LatitudeDeg;
double LongitudeDeg;
int Satellites;//補足した衛星の数


void begin();
bool GPSRead();
void CalcSun(double longitude_deg,double latitude_deg);

  private:
unsigned long cpuMSec;
long lastHour;
long lastMin;
long lastSec;
unsigned long lastMSec;

char gps_txt[SERIAL_BUFFSIZE];
const int monthday[13]={31,31,28,31,30,31,30,31,31,30,31,30,31};
const int monthday_l[13]={31,31,29,31,30,31,30,31,31,30,31,30,31};
int DecodeGPS();
void DecodeZDA();
void DecodeGLL();
void DecodeGSV();
void DecodeRMC();
int SkipComma(int i);
int SkipDot(int i);
void GMTtoJST(int timeDiff);
bool CheckTimeSts();

};


#endif
