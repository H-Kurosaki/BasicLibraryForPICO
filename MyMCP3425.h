#ifndef _MCP3425_H_
#define _MCP3425_H_

#include "Arduino.h"
#include "Wire.h"

// I²Cアドレス (A2=0, A1=0, A0=0の場合)
#define MCP3425_address 0x68

// 基準電圧
#define MCP3425_VREF    2.048

// コンフィギュレーションレジスタ
// bit 7: RDY (0=ready, 1=not ready) - 書き込み時は1でワンショット開始
// bit 6-5: Channel (00 - MCP3425では使用しない)
// bit 4: O/C (0=ワンショット, 1=連続変換)
// bit 3-2: Sample Rate (10=15SPS/16bit, 01=60SPS/14bit, 00=240SPS/12bit)
// bit 1-0: PGA Gain (00=x1, 01=x2, 10=x4, 11=x8)

// 精度別コンフィギュレーション定数
#define MCP3425_CONFIG_12BIT_CONTINUOUS 0b10010000  // 12bit, 連続変換, PGA x1
#define MCP3425_CONFIG_14BIT_CONTINUOUS 0b10010100  // 14bit, 連続変換, PGA x1
#define MCP3425_CONFIG_16BIT_CONTINUOUS 0b10011000  // 16bit, 連続変換, PGA x1

#define MCP3425_CONFIG_12BIT_ONESHOT    0b10000000  // 12bit, ワンショット, PGA x1
#define MCP3425_CONFIG_14BIT_ONESHOT    0b10000100  // 14bit, ワンショット, PGA x1
#define MCP3425_CONFIG_16BIT_ONESHOT    0b10001000  // 16bit, ワンショット, PGA x1

class MCP3425 {
  public:
    MCP3425(TwoWire &w);
    void begin(void);              // デフォルト16bitモード
    void begin(int bits);          // 精度指定モード (12, 14, 16)
    double readVolt(void);
    int16_t readADC(void);
    uint8_t readConfig(void);      // デバッグ用
    
  private:
    TwoWire *wire;
    uint8_t currentConfig;         // 現在のコンフィギュレーション
    int currentBits;               // 現在の精度 (12, 14, 16)
    double getLSB(void);           // LSB値を取得
    int32_t getMaxCode(void);      // 最大コード値を取得
};

#endif