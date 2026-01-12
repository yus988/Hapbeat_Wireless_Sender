#include "adjustParams.h"

// ボタン押下時に送信するデータ
// category,wearerID,devicePos,dataID,subid,c_leftPower,c_rightPower,playType
const uint8_t data_BtnA[] = {0, 99, 99, 1, 0, 100, 100, 0};
const uint8_t data_BtnB[] = {0, 99, 99, 5, 0, 50, 50, 1};
const uint8_t data_BtnC[] = {0, 99, 99, 99, 0, 0, 0, 2};

// const uint8_t data_BtnA[] = {0, 0, 99, 0, 0, 100, 100, 0};
// const uint8_t data_BtnB[] = {2, 99, 99, 5, 0, 50, 50, 0};
// const uint8_t data_BtnC[] = {2, 99, 99, 99, 0, 0, 0, 2};


// const uint8_t data_BtnB[] = {0, 0, 99, 5, 0, 50, 50, 1};
#ifdef ENABLE_COLOR_SENSOR

  // 各色のしきい値のインスタンス
  const ColorThreshold RED_THD = {140, 255, 0, 60, 0, 60};
  const ColorThreshold BLUE_THD = {0, 60, 0, 100, 100, 255};
  const ColorThreshold YELLOW_THD = {100, 130, 70, 100, 0, 60};

  // 色別のIDとボリュームのインスタンス
  const VibrationParams RED_PARAMS = {2, 17};
  const VibrationParams BLUE_PARAMS = {0, 35};
  const VibrationParams YELLOW_PARAMS = {1, 30};

  // 接続 / 切断時のLEDの色
  const RGB COLOR_CONNECTED = {0, 255, 0};
  const RGB COLOR_UNCONNECTED = {255, 0, 0};

  // Hapbeat用パラメータの定義（共通）
  const uint8_t CATEGORY = 0;
  const uint8_t WEARER_ID = 99;
  const uint8_t DEVICE_POS = 99;
  const uint8_t SUB_ID = 0;
  const uint8_t PLAY_CMD = 1;

  // ループ時間の変数
  const uint16_t SEND_WEBAPP_INTERVAL = 1000;
  const uint16_t COLOR_SENSOR_INTERVAL = 50;
  const uint16_t COLOR_CHANGE_INTERVAL = 5000;
  const uint32_t RETAIN_REFRESH_INTERVAL = 60000;

  // H/W 設定
  const uint8_t LED_BRIGHTNESS = 5;

#endif // ENABLE_COLOR_SENSOR

#ifdef ENABLE_ACCEL_TRIGGER

// 加速度トリガーの送信データ定義（閾値レベル別・ベースデータ）
// category, wearerID, devicePos, dataID, subid, c_leftPower, c_rightPower, playType
// ※subidとpowerは実行時に動的に設定される
const uint8_t data_AccelLow[]  = {0, 99, 99, 1, 0, 0, 0, 0};   // 低閾値用ベース
const uint8_t data_AccelMid[]  = {0, 99, 99, 1, 0, 0, 0, 0};   // 中閾値用ベース
const uint8_t data_AccelHigh[] = {0, 99, 99, 1, 0, 0, 0, 0};   // 高閾値用ベース

// 閾値レベルと送信データのマッピング
// { threshold, baseData, powerMin, powerMax }
// ※各レベルは threshold 以上、次レベルの threshold 未満で適用
const AccelTriggerData ACCEL_TRIGGER_DATA[] = {
  { 1.0f, data_AccelLow,  20,  60 },  // 0.5G以上 → Low  (power: 20-60)
  { 3.0f, data_AccelMid,  40,  80 },  // 1.5G以上 → Mid  (power: 40-80)
  { 5.0f, data_AccelHigh, 60, 100 }   // 3.0G以上 → High (power: 60-100)
};
const uint8_t ACCEL_TRIGGER_DATA_COUNT = sizeof(ACCEL_TRIGGER_DATA) / sizeof(ACCEL_TRIGGER_DATA[0]);

// 加速度トリガーの設定
// ※最小閾値はACCEL_TRIGGER_DATA[0].thresholdMinで定義
const AccelTriggerConfig ACCEL_CONFIG = {
  .axis = AccelAxis::AXIS_XYZ,        // 監視する軸 (AXIS_X, AXIS_Y, AXIS_Z, AXIS_XYZ)
  .deadTimeMs = 100,                  // 連続トリガー防止 (ms)
  .displayEnabled = true,             // ディスプレイ表示ON
  .displayRangeG = 5.0f,              // 波形表示範囲 0-5G
  .sampleIntervalMs = 5,              // サンプリング間隔 5ms (200Hz)
  .acSampleCount = 8,                 // AC計算用サンプル数（少ないほど高速応答）
  .subIdMax = 6,                      // subidの最大値（0〜6のランダム）
  .interpType = InterpolationType::LINEAR  // 補間タイプ (LINEAR, EXP, LOG)
};

#endif // ENABLE_ACCEL_TRIGGER

