/**
 * MAC_EEPROM.cpp
 *
 * Microchip 24AA025E48 / 24AA02E48 シリーズ
 * I2C MACアドレス書込済 EEPROM ライブラリ 実装
 */

#include "MyMAC_EEPROM24AA025E48.h"

// ================================================================
// コンストラクタ
// ================================================================
MAC_EEPROM::MAC_EEPROM(TwoWire &wire, MAC_EEPROM_Type type, uint8_t page_size)
    : _wire(wire),
      _addr(0x50),
      _type(type),
      _page_size(page_size),
      _last_error(MAC_EEPROM_Error::OK)
{}

// ================================================================
// begin: デバイス存在確認
// ================================================================
bool MAC_EEPROM::begin(uint8_t i2c_addr) {
    _addr = i2c_addr;
    if (!isConnected()) {
        _last_error = MAC_EEPROM_Error::DEVICE_NOT_FOUND;
        return false;
    }
    _last_error = MAC_EEPROM_Error::OK;
    return true;
}

// ================================================================
// isConnected: I2C ACK確認
// ================================================================
bool MAC_EEPROM::isConnected() {
    _wire.beginTransmission(_addr);
    return (_wire.endTransmission() == 0);
}

// ================================================================
// readMAC: EUI-48 読み出し (Section 9.1)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::readMAC(uint8_t buf[MACEEPROM_EUI48_LEN]) {
    return _read(MACEEPROM_EUI48_ADDR, buf, MACEEPROM_EUI48_LEN);
}

// ================================================================
// readMAC64: EUI-64 読み出し (Section 9.2)
//   EUI-48 デバイスの場合は EUI-64 へのカプセル化を自動実施
//   カプセル化: OUI(3B) + 0xFFFE(2B) + ExtID(3B) (Section 9.1.2)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::readMAC64(uint8_t buf[MACEEPROM_EUI64_LEN]) {
    if (_type == MAC_EEPROM_Type::E64) {
        // E64 デバイス: 0xF8〜0xFF を直接読む
        return _read(MACEEPROM_EUI64_ADDR, buf, MACEEPROM_EUI64_LEN);
    }

    // E48 デバイス: EUI-48 を読んでカプセル化
    uint8_t mac48[MACEEPROM_EUI48_LEN];
    MAC_EEPROM_Error err = _read(MACEEPROM_EUI48_ADDR, mac48, MACEEPROM_EUI48_LEN);
    if (err != MAC_EEPROM_Error::OK) return err;

    // EUI-48 → EUI-64 カプセル化
    buf[0] = mac48[0];  // OUI[0]
    buf[1] = mac48[1];  // OUI[1]
    buf[2] = mac48[2];  // OUI[2]
    buf[3] = 0xFF;      // 挿入値 (固定)
    buf[4] = 0xFE;      // 挿入値 (固定)
    buf[5] = mac48[3];  // ExtID[0]
    buf[6] = mac48[4];  // ExtID[1]
    buf[7] = mac48[5];  // ExtID[2]
    return MAC_EEPROM_Error::OK;
}

// ================================================================
// readByte: 1バイトランダムリード (Section 8.2)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::readByte(uint8_t mem_addr, uint8_t &out) {
    MAC_EEPROM_Error err = _read(mem_addr, &out, 1);
    return err;
}

// ================================================================
// readBytes: 複数バイトシーケンシャルリード (Section 8.3)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::readBytes(uint8_t mem_addr, uint8_t *buf, uint8_t len) {
    return _read(mem_addr, buf, len);
}

// ================================================================
// writeByte: 1バイトライト (Section 6.1)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::writeByte(uint8_t mem_addr, uint8_t data) {
    // 書き込み保護チェック (Section 6.3)
    if (mem_addr > MACEEPROM_USER_END) {
        _last_error = MAC_EEPROM_Error::WRITE_PROTECTED;
        return _last_error;
    }
    return _writePage(mem_addr, &data, 1);
}

// ================================================================
// writeBytes: 複数バイトページライト (Section 6.2)
//   ページ境界を自動検出して分割書き込み
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::writeBytes(uint8_t mem_addr,
                                         const uint8_t *buf, uint8_t len) {
    // 書き込み保護チェック
    if (mem_addr > MACEEPROM_USER_END) {
        _last_error = MAC_EEPROM_Error::WRITE_PROTECTED;
        return _last_error;
    }
    if ((uint16_t)mem_addr + len - 1 > MACEEPROM_USER_END) {
        _last_error = MAC_EEPROM_Error::WRITE_PROTECTED;
        return _last_error;
    }

    uint8_t written = 0;
    while (written < len) {
        uint8_t cur_addr  = mem_addr + written;
        // ページ内の残りバイト数を計算 (Section 6.2: ページ境界を跨げない)
        uint8_t page_offset   = cur_addr % _page_size;
        uint8_t space_in_page = _page_size - page_offset;
        uint8_t to_write      = min((uint8_t)(len - written), space_in_page);

        MAC_EEPROM_Error err = _writePage(cur_addr, buf + written, to_write);
        if (err != MAC_EEPROM_Error::OK) return err;

        written += to_write;
    }
    return MAC_EEPROM_Error::OK;
}

// ================================================================
// errorString: エラーコードを文字列に変換
// ================================================================
const char* MAC_EEPROM::errorString(MAC_EEPROM_Error err) {
    switch (err) {
        case MAC_EEPROM_Error::OK:               return "OK";
        case MAC_EEPROM_Error::DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
        case MAC_EEPROM_Error::NACK:             return "NACK";
        case MAC_EEPROM_Error::READ_MISMATCH:    return "READ_MISMATCH";
        case MAC_EEPROM_Error::WRITE_PROTECTED:  return "WRITE_PROTECTED";
        case MAC_EEPROM_Error::WRITE_TIMEOUT:    return "WRITE_TIMEOUT";
        case MAC_EEPROM_Error::INVALID_ADDRESS:  return "INVALID_ADDRESS";
        default:                                  return "UNKNOWN";
    }
}

// ================================================================
// macToString: MACアドレス → コロン区切り文字列
// ================================================================
void MAC_EEPROM::macToString(const uint8_t *mac, uint8_t len, char *buf) {
    uint8_t pos = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) buf[pos++] = ':';
        buf[pos++] = "0123456789ABCDEF"[(mac[i] >> 4) & 0x0F];
        buf[pos++] = "0123456789ABCDEF"[ mac[i]       & 0x0F];
    }
    buf[pos] = '\0';
}

// ================================================================
// _read: 内部読み出し実装
//   アドレスポインタセット (ダミーライト) → シーケンシャルリード
//   データシート Section 8.2/8.3
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::_read(uint8_t mem_addr, uint8_t *buf, uint8_t len) {
    // (1) アドレスポインタをセット
    //     endTransmission(false) でリピートスタートを発行
    _wire.beginTransmission(_addr);
    _wire.write(mem_addr);
    uint8_t err = _wire.endTransmission(false);
    if (err != 0) {
        _last_error = MAC_EEPROM_Error::NACK;
        return _last_error;
    }

    // (2) len バイト読み出しリクエスト
    uint8_t received = _wire.requestFrom(_addr, len);
    if (received != len) {
        _last_error = MAC_EEPROM_Error::READ_MISMATCH;
        return _last_error;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = _wire.read();
    }

    _last_error = MAC_EEPROM_Error::OK;
    return _last_error;
}

// ================================================================
// _writePage: 1ページ以内の書き込み実装 (Section 6.1/6.2)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::_writePage(uint8_t mem_addr,
                                         const uint8_t *buf, uint8_t len) {
    _wire.beginTransmission(_addr);
    _wire.write(mem_addr);
    for (uint8_t i = 0; i < len; i++) {
        _wire.write(buf[i]);
    }
    uint8_t err = _wire.endTransmission(true);  // STOP 発行 → 内部書き込み開始
    if (err != 0) {
        _last_error = MAC_EEPROM_Error::NACK;
        return _last_error;
    }

    // 書き込み完了待ち (ACKポーリング)
    return _waitWriteComplete();
}

// ================================================================
// _waitWriteComplete: ACKポーリング (Section 7.0)
//   書き込みサイクル中はデバイスが NACK を返す
//   ACK が返るまで繰り返す (最大 MACEEPROM_WRITE_TIMEOUT_MS)
// ================================================================
MAC_EEPROM_Error MAC_EEPROM::_waitWriteComplete() {
    uint32_t start = millis();
    while (millis() - start < MACEEPROM_WRITE_TIMEOUT_MS) {
        _wire.beginTransmission(_addr);
        if (_wire.endTransmission() == 0) {
            // ACK 返ってきた → 書き込み完了
            _last_error = MAC_EEPROM_Error::OK;
            return _last_error;
        }
        delayMicroseconds(200);
    }
    _last_error = MAC_EEPROM_Error::WRITE_TIMEOUT;
    return _last_error;
}
