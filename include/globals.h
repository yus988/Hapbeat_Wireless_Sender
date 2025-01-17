#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Wire.h>

// // // ESP32-S3判定とシリアルポート設定
// // #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ESP32S3)
// //   #define Serial USBSerial
// // #else
// //   #define Serial Serial
// // #endif

// // デバッグマクロ
// #ifdef ENABLE_DEBUG
//   #define DEBUG_PRINT(x) Serial.print(x)
//   #define DEBUG_PRINTLN(x) Serial.println(x)
//   #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
// #else
//   #define DEBUG_PRINT(x)
//   #define DEBUG_PRINTLN(x)
//   #define DEBUG_PRINTF(fmt, ...)
// #endif

// // シリアルポート初期化用関数
// inline void initSerialPort() {
//   Serial.begin(115200);
//   delay(2000);  // シリアルポートの安定化待ち
// }

#endif  // GLOBALS_H