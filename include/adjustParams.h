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

#endif // ENABLE_COLOR_SENSOR

#endif // ADJ_PARAMS_H
