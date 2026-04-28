/**
 * MAC_EEPROM.h
 *
 * Microchip 24AA025E48 / 24AA02E48 シリーズ
 * I2C MACアドレス書込済 EEPROM ライブラリ
 *
 * 対応デバイス:
 *   24AA02E48  / 24AA02E64   (アドレスピンなし, 8バイトページ)
 *   24AA025E48 / 24AA025E64  (A0/A1/A2付き, 16バイトページ)
 *
 * データシート: DS20002124K (Microchip Technology)
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>

// ---- デバイス種別 ----
enum class MAC_EEPROM_Type : uint8_t {
    E48,   // EUI-48 (6バイト MACアドレス)
    E64,   // EUI-64 (8バイト MACアドレス)
};

// ---- エラーコード ----
enum class MAC_EEPROM_Error : uint8_t {
    OK = 0,
    DEVICE_NOT_FOUND,   // デバイス未検出
    NACK,               // I2C NACK
    READ_MISMATCH,      // 受信バイト数不一致
    WRITE_PROTECTED,    // 書き込み保護アドレスへの書き込み試行
    WRITE_TIMEOUT,      // 書き込み完了待ちタイムアウト
    INVALID_ADDRESS,    // アドレス範囲外
};

// ---- メモリマップ (データシート Section 9.0) ----
static const uint8_t MACEEPROM_USER_START  = 0x00;
static const uint8_t MACEEPROM_USER_END    = 0x7F;
static const uint8_t MACEEPROM_PROT_START  = 0x80;
static const uint8_t MACEEPROM_PROT_END    = 0xFF;
static const uint8_t MACEEPROM_EUI48_ADDR  = 0xFA;  // EUI-48 格納先頭
static const uint8_t MACEEPROM_EUI64_ADDR  = 0xF8;  // EUI-64 格納先頭
static const uint8_t MACEEPROM_EUI48_LEN   = 6;
static const uint8_t MACEEPROM_EUI64_LEN   = 8;

// ---- ページサイズ (Section 6.2) ----
static const uint8_t MACEEPROM_PAGE_SIZE_025 = 16;  // 24AA025E48/E64
static const uint8_t MACEEPROM_PAGE_SIZE_02  = 8;   // 24AA02E48/E64

// ---- 書き込みサイクル最大待ち時間 ms (Section 6.1, TWC max=5ms) ----
static const uint32_t MACEEPROM_WRITE_TIMEOUT_MS = 10;


class MAC_EEPROM {
public:
    /**
     * コンストラクタ
     * @param wire      使用する TwoWire インスタンス (省略時 Wire)
     * @param type      デバイス種別 (E48 or E64)
     * @param page_size ページサイズ (8 or 16バイト)
     */
    MAC_EEPROM(TwoWire &wire     = Wire,
               MAC_EEPROM_Type type      = MAC_EEPROM_Type::E48,
               uint8_t         page_size = MACEEPROM_PAGE_SIZE_025);

    /**
     * 初期化
     * @param i2c_addr 7ビットI2Cアドレス (A2/A1/A0=0b000 → 0x50)
     * @return true: デバイス検出OK
     */
    bool begin(uint8_t i2c_addr = 0x50);

    // ---- MAC アドレス読み出し ----

    /**
     * EUI-48 MACアドレスを読み出す (6バイト)
     * @param buf 読み出し先バッファ (6バイト以上)
     * @return エラーコード
     */
    MAC_EEPROM_Error readMAC(uint8_t buf[MACEEPROM_EUI48_LEN]);

    /**
     * EUI-64 MACアドレスを読み出す (8バイト)
     * EUI-48デバイスで呼んだ場合は自動的に encapsulate して返す
     * @param buf 読み出し先バッファ (8バイト以上)
     * @return エラーコード
     */
    MAC_EEPROM_Error readMAC64(uint8_t buf[MACEEPROM_EUI64_LEN]);

    // ---- 汎用メモリアクセス ----

    /**
     * 任意アドレスから1バイト読み出し (ランダムリード)
     * @param mem_addr メモリアドレス (0x00〜0xFF)
     * @param out      読み出し結果格納先
     * @return エラーコード
     */
    MAC_EEPROM_Error readByte(uint8_t mem_addr, uint8_t &out);

    /**
     * 任意アドレスから複数バイト連続読み出し (シーケンシャルリード)
     * @param mem_addr 先頭メモリアドレス
     * @param buf      読み出し先バッファ
     * @param len      読み出しバイト数
     * @return エラーコード
     */
    MAC_EEPROM_Error readBytes(uint8_t mem_addr, uint8_t *buf, uint8_t len);

    /**
     * 1バイト書き込み (バイトライト) — ユーザー領域のみ
     * @param mem_addr メモリアドレス (0x00〜0x7F)
     * @param data     書き込みデータ
     * @return エラーコード
     */
    MAC_EEPROM_Error writeByte(uint8_t mem_addr, uint8_t data);

    /**
     * 複数バイト書き込み (ページライト) — ユーザー領域のみ
     * ページ境界を跨ぐ場合は自動分割して書き込む
     * @param mem_addr 先頭メモリアドレス (0x00〜0x7F)
     * @param buf      書き込みデータ
     * @param len      書き込みバイト数
     * @return エラーコード
     */
    MAC_EEPROM_Error writeBytes(uint8_t mem_addr, const uint8_t *buf, uint8_t len);

    // ---- ユーティリティ ----

    /**
     * デバイス存在確認
     * @return true: 応答あり
     */
    bool isConnected();

    /**
     * エラーコードを文字列に変換
     */
    static const char* errorString(MAC_EEPROM_Error err);

    /**
     * MACアドレスをコロン区切りの文字列に変換
     * @param mac  MACアドレスバッファ
     * @param len  バイト数 (6 or 8)
     * @param buf  出力バッファ (6バイトなら18文字以上, 8バイトなら24文字以上)
     */
    static void macToString(const uint8_t *mac, uint8_t len, char *buf);

    /**
     * 直前のエラーコードを取得
     */
    MAC_EEPROM_Error lastError() const { return _last_error; }

private:
    TwoWire          &_wire;
    uint8_t           _addr;
    MAC_EEPROM_Type   _type;
    uint8_t           _page_size;
    MAC_EEPROM_Error  _last_error;

    // 内部: アドレスポインタセット + 連続読み出し (Section 8.2/8.3)
    MAC_EEPROM_Error _read(uint8_t mem_addr, uint8_t *buf, uint8_t len);

    // 内部: 1ページ分の書き込み (Section 6.1/6.2)
    MAC_EEPROM_Error _writePage(uint8_t mem_addr, const uint8_t *buf, uint8_t len);

    // 内部: ACKポーリングによる書き込み完了待ち (Section 7.0)
    MAC_EEPROM_Error _waitWriteComplete();
};
