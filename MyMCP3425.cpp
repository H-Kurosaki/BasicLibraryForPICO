#include "MyMCP3425.h"

MCP3425::MCP3425(TwoWire &w) {
  wire = &w;
}

void MCP3425::begin(void) {
    wire->beginTransmission(MCP3425_address);
    wire->write(MCP3425_configRegister);
    uint8_t error = wire->endTransmission();
    
    if (error != 0) {
        Serial.print("MCP3425 initialization error: ");
        Serial.println(error);
    }
    
    delay(100);  // 初期化後の安定化時間
}

double MCP3425::readVolt(void) {
    int16_t adcValue = readADC();
    // 16ビット符号付きの範囲: -32768 to +32767
    // フルスケール範囲: ±2.048V
    return (double)adcValue * MCP3425_VREF / 32768.0;
}

int16_t MCP3425::readADC(void) {
    // 3バイト読み取り（データ2バイト + コンフィギュレーション1バイト）
    wire->requestFrom(MCP3425_address, 3);
    
    // データが利用可能になるまで待機
    int timeout = 0;
    while(wire->available() < 3 && timeout < 100) {
        delay(1);
        timeout++;
    }
    
    if (wire->available() < 3) {
        Serial.println("MCP3425: Timeout reading data");
        return 0;
    }
    
    uint8_t msb = wire->read();
    uint8_t lsb = wire->read();
    uint8_t config = wire->read();
    
    // RDYビットをチェック（bit 7が0なら変換完了）
    if (config & 0x80) {
        // まだ変換中の場合は待機してから再試行
        delay(100);  // 16ビットモード(15SPS)なので約67ms必要
        return readADC();
    }
    
    // 16ビット符号付き整数として結合
    int16_t result = (int16_t)((msb << 8) | lsb);
    return result;
}

// デバッグ用：生のコンフィギュレーションバイトを読み取り
uint8_t MCP3425::readConfig(void) {
    wire->requestFrom(MCP3425_address, 3);
    
    if (wire->available() >= 3) {
        wire->read();  // MSB
        wire->read();  // LSB
        return wire->read();  // Config
    }
    return 0;
}
