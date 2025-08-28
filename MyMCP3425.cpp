#include "MyMCP3425.h"

MCP3425::MCP3425(TwoWire &w) {
  wire = &w;
  currentConfig = MCP3425_CONFIG_16BIT_CONTINUOUS;  // デフォルト16bit
  currentBits = 16;
}

void MCP3425::begin(void) {
    // デフォルト16bitモードで初期化
    begin(16);
}

void MCP3425::begin(int bits) {
    // 精度に応じてコンフィギュレーションを設定
    switch(bits) {
        case 12:
            currentConfig = MCP3425_CONFIG_12BIT_CONTINUOUS;
            currentBits = 12;
            break;
        case 14:
            currentConfig = MCP3425_CONFIG_14BIT_CONTINUOUS;
            currentBits = 14;
            break;
        case 16:
            currentConfig = MCP3425_CONFIG_16BIT_CONTINUOUS;
            currentBits = 16;
            break;
        default:
            // 無効な精度の場合は16bitにフォールバック
            Serial.print("Invalid resolution: ");
            Serial.print(bits);
            Serial.println(". Using 16-bit mode.");
            currentConfig = MCP3425_CONFIG_16BIT_CONTINUOUS;
            currentBits = 16;
            break;
    }
    
    // デバイスにコンフィギュレーションを書き込み
    wire->beginTransmission(MCP3425_address);
    wire->write(currentConfig);
    uint8_t error = wire->endTransmission();
    
    if (error != 0) {
        Serial.print("MCP3425 initialization error: ");
        Serial.println(error);
    }
    
    delay(50);  // 初期化後の安定化時間を短縮
    
    Serial.print("MCP3425 initialized with ");
    Serial.print(currentBits);
    Serial.println("-bit resolution");
}

double MCP3425::readVolt(void) {
    int16_t adcValue = readADC();
    
    // 各精度に応じて電圧に変換
    double voltage;
    switch(currentBits) {
        case 12:
            // 12bitの場合、上位4bitは繰り返しMSBなので無視
            // D11が符号ビット、実際の値は12bit幅で-2048~+2047
            // 12bitの符号付き値として扱い、16bitスケールに拡張
            adcValue = adcValue & 0x0FFF; // 下位12bitを抽出
            if (adcValue & 0x0800) {      // 符号ビット（D11）が1なら
                adcValue |= 0xF000;       // 上位4bitを1で埋める（符号拡張）
            }
            voltage = (double)adcValue * MCP3425_VREF / 2048.0;
            break;
            
        case 14:
            // 14bitの場合、上位2bitは繰り返しMSBなので無視
            // D13が符号ビット、実際の値は14bit幅で-8192~+8191
            // 14bitの符号付き値として扱い、16bitスケールに拡張
            adcValue = adcValue & 0x3FFF; // 下位14bitを抽出
            if (adcValue & 0x2000) {      // 符号ビット（D13）が1なら
                adcValue |= 0xC000;       // 上位2bitを1で埋める（符号拡張）
            }
            voltage = (double)adcValue * MCP3425_VREF / 8192.0;
            break;
            
        case 16:
            // 16bitの場合、フル範囲使用: -32768 to +32767
            voltage = (double)adcValue * MCP3425_VREF / 32768.0;
            break;
            
        default:
            voltage = 0.0;
            break;
    }
    
    return voltage;
}

int16_t MCP3425::readADC(void) {
    // 最初にRDYビットをチェック（前回の変換が完了している可能性）
    wire->requestFrom(MCP3425_address, 3);
    
    // データが利用可能になるまで待機（短縮されたタイムアウト）
    int timeout = 0;
    while(wire->available() < 3 && timeout < 50) {  // タイムアウトを短縮
        delayMicroseconds(500);  // delayをdelayMicrosecondsに変更
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
        // まだ変換中の場合は最小限の待機時間で再試行
        int waitTimeMicros;
        
        // 精度に応じた最小待機時間をマイクロ秒で設定
        switch(currentBits) {
            case 12: waitTimeMicros = 1500; break;  // 240SPS -> 約4.2ms, マージン1.5ms
            case 14: waitTimeMicros = 6000; break;  // 60SPS  -> 約16.7ms, マージン6ms  
            case 16: waitTimeMicros = 30000; break; // 15SPS  -> 約66.7ms, マージン30ms
        }
        
        delayMicroseconds(waitTimeMicros);
        return readADC();  // 再帰呼び出し
    }
    
    // 16ビット符号付き整数として結合
    int16_t result = (int16_t)((msb << 8) | lsb);
    
    // データシートによると、未使用ビットは符号拡張される
    // 12bitと14bitモードでは、そのまま返す（readVolt側で適切に処理）
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

double MCP3425::getLSB(void) {
    // データシートのTable 4-1に基づく
    switch(currentBits) {
        case 12: return 0.001;     // 1.0 mV (2*2.048V / 2^12)
        case 14: return 0.00025;   // 0.25 mV (2*2.048V / 2^14)
        case 16: return 0.0000625; // 62.5 μV (2*2.048V / 2^16)
        default: return 0.0;
    }
}

int32_t MCP3425::getMaxCode(void) {
    switch(currentBits) {
        case 12: return 2047;
        case 14: return 8191;
        case 16: return 32767;
        default: return 0;
    }
}
