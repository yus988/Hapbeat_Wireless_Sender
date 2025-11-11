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



