/*
 *SHT3x,SHT4x両対応ライブラリ
 *20250814 H.kurosaki
 * 対応確認済みマイコン:
 * - Arduino UNO
 * - Raspberry Pi Pico (複数I2Cバス対応)
 */

#ifndef _MYSHT3X4X_H_
#define _MYSHT3X4X_H_

#include "Arduino.h"
#include "Wire.h"



#define SHT3x_ADDR    0x44

class SHT3x {
  public:
    SHT3x();
    SHT3x(TwoWire &w);
    bool autoRead(); 
    bool begin(unsigned char i2caddr = SHT3x_ADDR);
    bool startMeasure(void);
    bool getTempHumid(void);
    double humidity, temp,humiddiff;
    int retryCount;
  private:
	TwoWire *wire;
    int writeCommand(unsigned short cmd);
    bool reset(void);
    unsigned char crc8Dallas(const unsigned char *data, int len);
    unsigned char _i2caddr;
	bool SensorSts;
	bool measureFlag;
};


#define SHT4X_ADDR    0x44

class SHT4x {
public:
  // コンストラクタ（SHT3xと同じインターフェース）
  SHT4x();
  SHT4x(TwoWire &w);
  
  // メイン関数（SHT3xと完全互換）
  bool autoRead();
  bool begin(unsigned char i2caddr = SHT4X_ADDR);
  bool startMeasure(void);
  bool getTempHumid(void);
  
  // パブリック変数（SHT3xと同じ）
  double humidity, temp, humiddiff;
  int retryCount;
  
  bool reset(void);
  bool readSerialNumber(uint32_t* serialNumber);
  void setPrecision(unsigned char precision);  // 0:高精度, 1:中精度, 2:低精度
  unsigned char getPrecision();
  bool isConnected();
  

private:
  TwoWire *wire;
  unsigned char _i2caddr;
  unsigned char _precision;  // 0:高精度, 1:中精度, 2:低精度
  bool _initialized;
  bool SensorSts;
  bool measureFlag;

	
  // プライベートメソッド
  unsigned char crc8Dallas(const unsigned char *data, int len);
  unsigned char getPrecisionCommand();
};

#endif
