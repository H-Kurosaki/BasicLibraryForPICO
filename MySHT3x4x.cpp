#include "MySHT3x4x.h"


SHT3x::SHT3x() {
	wire = &Wire;
}

SHT3x::SHT3x(TwoWire &w) {
  wire = &w;
}

//----------------------推奨読み出し間隔16ms以上
bool SHT3x::autoRead()
{
    if(!measureFlag)
      {
     //センサ計測開始
        startMeasure();
        measureFlag=true;
       }
    else if(getTempHumid())
      {
     //センサ読み出し成功
         double t=temp;
         double rh=humidity;
          //飽差計算
          double humidpress=6.1078*pow(10,(7.5*t/(t+237.3)));
          double humidvol=217*humidpress/(t+273.15);
          humiddiff=(100-rh)*humidvol/100;
          measureFlag=false;
          SensorSts=true;
        retryCount=0;
        }
     else
     {
     //センサ読み出し失敗
     measureFlag=false;
     SensorSts=false;
     humidity=-999;
     temp=-999;
     humiddiff=-999;
      begin(_i2caddr);
      if(retryCount<128){retryCount++;}
     }
  return SensorSts;
}

bool SHT3x::begin(unsigned char i2caddr) {
	SensorSts=false;
	measureFlag=false;
	_i2caddr = i2caddr;
  return reset();
}

bool SHT3x::startMeasure(void)
{
int ret=writeCommand(0x2400);//MEAS_HIGHREP
if(ret==0){return true;}
  return false;
}


bool SHT3x::getTempHumid(void) {

  unsigned char i2cbuffer[6];
 
  wire->requestFrom(_i2caddr, (unsigned char)6);
  if (wire->available() != 6) 
    return false;
  for (unsigned char i=0; i<6; i++) {
    i2cbuffer[i] = wire->read();
  }
  unsigned short SensorT, SensorRH;
  SensorT = i2cbuffer[0];
  SensorT <<= 8;
  SensorT |= i2cbuffer[1];

  if (i2cbuffer[2] != crc8Dallas(i2cbuffer, 2)) return false;

  SensorRH = i2cbuffer[3];
  SensorRH <<= 8;
  SensorRH |= i2cbuffer[4];

  if (i2cbuffer[5] != crc8Dallas(i2cbuffer+3, 2)) return false;

  temp = (double)SensorT*175.0/65535.0-45.0;
  humidity=(double)SensorRH*100.0/65535.0;

  return true;
}

bool SHT3x::reset(void) {
int ret=writeCommand(0x30A2);//reset command
  delay(10);
if(ret==0){return true;}
  return false;
}

unsigned char SHT3x::crc8Dallas(const unsigned char *data, int len) {
  unsigned char crcval(0xFF);
  
  for ( int j = len; j; --j ) {
      crcval ^= *data++;

      for ( int i = 8; i; --i ) {
  crcval = ( crcval & 0x80 )
    ? (crcval << 1) ^ 0x31//polynomial value
    : (crcval << 1);
      }
  }
  return crcval; 
}


int SHT3x::writeCommand(unsigned short cmd) {
  wire->beginTransmission(_i2caddr);
  wire->write(cmd >> 8);
  wire->write(cmd & 0xFF);
  return wire->endTransmission();      
}

////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------
////------------------------------------------------------------------------------------------------------------

// コンストラクタ（遅延初期化 - Wireポインタは後で設定）
SHT4x::SHT4x() {
  wire = nullptr;       // 遅延初期化（begin()で設定）
  _precision = 0;       // 高精度
  retryCount = 0;
  _initialized = false;
  temp = -999;
  humidity = -999;
  humiddiff = -999;
}

// コンストラクタ（TwoWire指定）
SHT4x::SHT4x(TwoWire &w) {
  wire = &w;           // 明示的に指定された場合は直接設定
  _precision = 0;
  retryCount = 0;
  _initialized = false;
  temp = -999;
  humidity = -999;
  humiddiff = -999;
}

// 初期化（SHT3xと同じインターフェース）
bool SHT4x::begin(unsigned char i2caddr) {
  _i2caddr = i2caddr;
	SensorSts=false;
	measureFlag=false;

  // Wireが設定されていない場合は標準Wireを使用（遅延初期化）
  if (wire == nullptr) {
    wire = &Wire;
  }

  
  // 基本的な接続確認
  wire->beginTransmission(_i2caddr);
  if (wire->endTransmission() != 0) {
    return false;
  }

   reset();
   delay(1);

  
  _initialized = true;
  return true;
}

//---------------------------------------------------------------自動読み出し
//推奨読み出し間隔10ms以上
bool SHT4x::autoRead() {
  
  if (!_initialized) return false;

  if (!measureFlag) {
    // センサ計測開始
    if (startMeasure()) {
      measureFlag = true;
    } else {
      // 計測開始失敗
      SensorSts = false;
      humidity = -999;
      temp = -999;
      humiddiff = -999;
	  if (retryCount < 128){retryCount++;}
	  // 再初期化試行
      begin(_i2caddr);
      return false;
    }
  } else if (getTempHumid()) {
    // センサ読み出し成功
    double t = temp;
    double rh = humidity;
    
    // 飽差計算（SHT3xと同じ計算式）
    double humidpress = 6.1078 * pow(10, (7.5 * t / (t + 237.3)));
    double humidvol = 217 * humidpress / (t + 273.15);
    humiddiff = (100 - rh) * humidvol / 100;
    
    measureFlag = false;
    SensorSts = true;
    retryCount = 0;
  } else {
    // センサ読み出し失敗
    measureFlag = false;
    SensorSts = false;
    humidity = -999;
    temp = -999;
    humiddiff = -999;
    
    // 再初期化試行
    begin(_i2caddr);
    if (retryCount < 128) {retryCount++;}
  }
  
  return SensorSts;
}

bool SHT4x::startMeasure(void) {
  if (!_initialized || !wire) return false;
  
  wire->beginTransmission(_i2caddr);
  wire->write(getPrecisionCommand());
  return (wire->endTransmission() == 0);
}

// ★★★ 重要：換算式修正版 ★★★
bool SHT4x::getTempHumid(void) {
  if (!_initialized || !wire) return false;
  
  wire->requestFrom(_i2caddr, (uint8_t)6);
  delay(5); // データ受信待機
  
  if (wire->available() != 6) {
    // バッファクリア
    while (wire->available()) {
      wire->read();
    }
    return false;
  }
  
  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = wire->read();
  }
  
  // CRCチェック
  if (data[2] != crc8Dallas(data, 2)) return false;
  if (data[5] != crc8Dallas(data + 3, 2)) return false;
  
  // 値変換
  uint16_t tempRaw = (data[0] << 8) | data[1];
  uint16_t humRaw = (data[3] << 8) | data[4];
  
  temp = -45.0 + 175.0 * (double)tempRaw / 65535.0;
  humidity = -6.0 + 125.0 * (double)humRaw / 65535.0;
  
  // 湿度範囲制限
  if (humidity > 100.0) humidity = 100.0;
  if (humidity < 0.0) humidity = 0.0;
  
  return true;
}

bool SHT4x::reset(void) {
  if (!_initialized || !wire) return false;
  
  wire->beginTransmission(_i2caddr);
  wire->write(0x94);  // ソフトリセットコマンド
  bool result = (wire->endTransmission() == 0);
  delay(1); // リセット待機
  return result;
}

bool SHT4x::readSerialNumber(uint32_t* serialNumber) {
  if (!_initialized || !wire) return false;
  
  wire->beginTransmission(_i2caddr);
  wire->write(0x89);  // シリアル番号読み取りコマンド
  if (wire->endTransmission() != 0) return false;
  
  delay(1);
  
  wire->requestFrom(_i2caddr, (uint8_t)6);
  if (wire->available() != 6) return false;
  
  uint8_t buffer[6];
  for (int i = 0; i < 6; i++) {
    buffer[i] = wire->read();
  }
  
  uint16_t serial_high = (buffer[0] << 8) | buffer[1];
  uint16_t serial_low = (buffer[3] << 8) | buffer[4];
  
  if (buffer[2] != crc8Dallas(buffer, 2)) return false;
  if (buffer[5] != crc8Dallas(buffer + 3, 2)) return false;
  
  *serialNumber = ((uint32_t)serial_high << 16) | serial_low;
  return true;
}

void SHT4x::setPrecision(unsigned char precision) {
  if (precision <= 2) {
    _precision = precision;
  }
}

unsigned char SHT4x::getPrecision() {
  return _precision;
}

bool SHT4x::isConnected() {
  if (!wire) return false;
  wire->beginTransmission(_i2caddr);
  return (wire->endTransmission() == 0);
}

// CRC8計算（SHT3xと同じ実装）
unsigned char SHT4x::crc8Dallas(const unsigned char *data, int len) {
  unsigned char crcval(0xFF);
  
  for (int j = len; j; --j) {
    crcval ^= *data++;
    
    for (int i = 8; i; --i) {
      crcval = (crcval & 0x80)
        ? (crcval << 1) ^ 0x31  // polynomial value
        : (crcval << 1);
    }
  }
  return crcval;
}

// 精度コマンド取得（プライベート）
unsigned char SHT4x::getPrecisionCommand() {
  switch (_precision) {
    case 0: return 0xFD;  // 高精度
    case 1: return 0xF6;  // 中精度
    case 2: return 0xE0;  // 低精度
    default: return 0xFD;
  }
}

