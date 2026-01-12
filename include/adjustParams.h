#ifndef ADJ_PARAMS_H
#define ADJ_PARAMS_H

#include <stdint.h>

// ボタン押下時のコマンド調整
extern const uint8_t data_BtnA[];
extern const uint8_t data_BtnB[];
extern const uint8_t data_BtnC[];

#ifdef ENABLE_COLOR_SENSOR

// カラーコンフィグ
struct ColorThreshold {
  uint8_t rMin;
  uint8_t rMax;
  uint8_t gMin;
  uint8_t gMax;
  uint8_t bMin;
  uint8_t bMax;
};

extern const ColorThreshold RED_THD;
extern const ColorThreshold BLUE_THD;
extern const ColorThreshold YELLOW_THD;

// VibrationParams 構造体の定義
struct VibrationParams {
  uint8_t id;
  uint8_t vol;
};

extern const VibrationParams RED_PARAMS;
extern const VibrationParams BLUE_PARAMS;
extern const VibrationParams YELLOW_PARAMS;

// RGB値を保持する構造体の定義
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

extern const RGB COLOR_CONNECTED;
extern const RGB COLOR_UNCONNECTED;
  #define CREATE_CRGB(color) CRGB((color).r, (color).g, (color).b)

// H/W 設定
extern const uint8_t LED_BRIGHTNESS;

// ループ時間の変数
extern const uint16_t SEND_WEBAPP_INTERVAL;
extern const uint16_t COLOR_SENSOR_INTERVAL;
extern const uint16_t COLOR_CHANGE_INTERVAL;
extern const uint32_t RETAIN_REFRESH_INTERVAL;

// Hapbeat用パラメータの定義（共通）
extern const uint8_t CATEGORY;
extern const uint8_t WEARER_ID;
extern const uint8_t DEVICE_POS;
extern const uint8_t SUB_ID;
extern const uint8_t PLAY_CMD;

#endif  // ENABLE_COLOR_SENSOR

#ifdef ENABLE_ACCEL_TRIGGER

// 加速度トリガーの軸選択
enum class AccelAxis : uint8_t {
  AXIS_X = 0,
  AXIS_Y = 1,
  AXIS_Z = 2,
  AXIS_XYZ = 3  // 合成加速度 (sqrt(x^2 + y^2 + z^2))
};

// 補間タイプ
enum class InterpolationType : uint8_t {
  LINEAR = 0,   // 線形補間
  EXP = 1,      // 指数補間
  LOG = 2       // 対数補間
};

// 加速度トリガーの設定構造体
struct AccelTriggerConfig {
  AccelAxis axis;           // 監視する軸
  uint16_t deadTimeMs;      // 連続トリガー防止のデッドタイム (ms)
  bool displayEnabled;      // ディスプレイ表示のON/OFF
  float displayRangeG;      // 波形表示の範囲 (±G)
  uint8_t sampleIntervalMs; // サンプリング間隔 (ms)
  uint8_t acSampleCount;    // AC計算用サンプル数（少ないほど高速応答）
  uint8_t subIdMax;         // subidの最大値（0〜この値のランダム）
  InterpolationType interpType; // 補間タイプ
};

// 閾値ごとの送信データ定義用構造体
struct AccelTriggerData {
  float threshold;         // この閾値以上で適用（次のレベルの閾値未満まで）
  const uint8_t* data;     // 送信するベースデータ
  uint8_t powerMin;        // このレベルの最小パワー
  uint8_t powerMax;        // このレベルの最大パワー
};

// 外部変数宣言
extern const AccelTriggerConfig ACCEL_CONFIG;
extern const AccelTriggerData ACCEL_TRIGGER_DATA[];
extern const uint8_t ACCEL_TRIGGER_DATA_COUNT;

// 送信データの定義（閾値レベル別）
extern const uint8_t data_AccelLow[];
extern const uint8_t data_AccelMid[];
extern const uint8_t data_AccelHigh[];

#endif  // ENABLE_ACCEL_TRIGGER

#endif  // ADJ_PARAMS_H
