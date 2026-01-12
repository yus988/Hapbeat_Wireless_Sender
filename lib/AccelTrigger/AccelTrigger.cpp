// AccelTrigger.cpp
// M5StickC PLUS2用 加速度トリガー機能

#include "AccelTrigger.h"

#ifdef ENABLE_ACCEL_TRIGGER

#include <Arduino.h>
#include <M5Unified.h>
#include <espnow_manager.h>
#include "adjustParams.h"

namespace AccelTrigger {

// 波形表示用のバッファ（横向き: 240x135、幅240に合わせる）
static const uint16_t WAVE_BUFFER_SIZE = 200;  // 波形表示用のサンプル数
static float waveBuffer[WAVE_BUFFER_SIZE];
static uint16_t waveIndex = 0;
static uint32_t lastTriggerTime = 0;
static uint32_t lastSampleTime = 0;
static uint32_t triggerCount = 0;

// AC計算用バッファ（DC成分除去用）
static const uint8_t AC_BUFFER_MAX = 16;  // 最大サンプル数
static float acBuffer[AC_BUFFER_MAX];
static uint8_t acBufferIndex = 0;
static float dcOffset = 0.0f;

// 送信履歴表示用
static char lastSentLevel[8] = "";
static uint8_t lastSentPower = 0;
static uint32_t lastSentTime = 0;

// ダブルバッファリング用スプライト
static M5Canvas canvas(&M5.Display);
static bool canvasInitialized = false;

// 生の加速度値を取得する関数（軸選択のみ）
static float getRawAccelValue(float ax, float ay, float az) {
  switch (ACCEL_CONFIG.axis) {
    case AccelAxis::AXIS_X:
      return ax;
    case AccelAxis::AXIS_Y:
      return ay;
    case AccelAxis::AXIS_Z:
      return az;
    case AccelAxis::AXIS_XYZ:
    default:
      return sqrtf(ax * ax + ay * ay + az * az);
  }
}

// DC成分を更新し、AC成分（絶対値）を返す
static float updateAcValue(float rawValue) {
  uint8_t sampleCount = min(ACCEL_CONFIG.acSampleCount, AC_BUFFER_MAX);
  
  // バッファに追加
  acBuffer[acBufferIndex] = rawValue;
  acBufferIndex = (acBufferIndex + 1) % sampleCount;
  
  // DC成分（移動平均）を計算
  float sum = 0.0f;
  for (uint8_t i = 0; i < sampleCount; i++) {
    sum += acBuffer[i];
  }
  dcOffset = sum / sampleCount;
  
  // AC成分 = 生値 - DC成分（絶対値で返す）
  return fabsf(rawValue - dcOffset);
}

// 補間関数（加速度値から0-1の正規化値を計算）
static float interpolate(float value, float minVal, float maxVal, InterpolationType type) {
  if (maxVal <= minVal) return 0.0f;
  
  // 0-1に正規化
  float t = (value - minVal) / (maxVal - minVal);
  t = constrain(t, 0.0f, 1.0f);
  
  switch (type) {
    case InterpolationType::EXP:
      // 指数補間（ゆっくり上がって急に上昇）
      return t * t;
    case InterpolationType::LOG:
      // 対数補間（急に上がってゆっくり）
      return sqrtf(t);
    case InterpolationType::LINEAR:
    default:
      return t;
  }
}

// 閾値に応じた送信データとレベル情報を取得し、パワーを計算
// 逆順でチェックし、最も高い閾値を超えるレベルを選択
static const uint8_t* getTriggerDataWithPower(float accelValue, const char** levelName, 
                                               uint8_t* outPower, uint8_t* levelIndex) {
  // 逆順でループ（最も高い閾値からチェック）
  for (int8_t i = ACCEL_TRIGGER_DATA_COUNT - 1; i >= 0; i--) {
    float threshold = ACCEL_TRIGGER_DATA[i].threshold;
    
    if (accelValue >= threshold) {
      // レベル名を設定
      if (i == 0) *levelName = "Low";
      else if (i == 1) *levelName = "Mid";
      else *levelName = "High";
      
      *levelIndex = i;
      
      // パワーを補間計算
      // 次のレベルの閾値を上限とする（最後のレベルは閾値+2.0Gを仮の上限）
      float nextThreshold = (i < ACCEL_TRIGGER_DATA_COUNT - 1) 
                            ? ACCEL_TRIGGER_DATA[i + 1].threshold 
                            : threshold + 2.0f;
      float t = interpolate(accelValue, threshold, nextThreshold, ACCEL_CONFIG.interpType);
      
      uint8_t powerMin = ACCEL_TRIGGER_DATA[i].powerMin;
      uint8_t powerMax = ACCEL_TRIGGER_DATA[i].powerMax;
      *outPower = powerMin + (uint8_t)(t * (powerMax - powerMin));
      
      return ACCEL_TRIGGER_DATA[i].data;
    }
  }
  *levelName = "";
  *outPower = 0;
  *levelIndex = 0;
  return nullptr;
}

// オシロスコープ風の波形表示（横向き、ダブルバッファリング）
static void drawWaveform() {
  if (!ACCEL_CONFIG.displayEnabled) return;
  if (!canvasInitialized) return;
  
  int16_t screenW = canvas.width();   // 240
  int16_t screenH = canvas.height();  // 135
  
  // グラフ領域の設定
  int16_t infoHeight = 24;  // 上部情報表示エリア
  int16_t graphLeft = 5;
  int16_t graphTop = infoHeight;
  int16_t graphWidth = screenW - graphLeft - 20;  // 右側にラベル用スペース
  int16_t graphHeight = screenH - graphTop - 20;  // 下部に送信履歴表示エリア
  
  // 画面クリア
  canvas.fillScreen(TFT_BLACK);
  
  // ステータス表示（上部）
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE);
  canvas.setCursor(2, 2);
  
  const char* axisName[] = {"X", "Y", "Z", "XYZ"};
  canvas.printf("Axis:%s  Trig:%lu  DC:%.2f", 
                axisName[(uint8_t)ACCEL_CONFIG.axis], triggerCount, dcOffset);
  
  // 現在の加速度値を表示（AC値）
  canvas.setCursor(2, 12);
  float currentValue = (waveIndex > 0) ? waveBuffer[(waveIndex - 1 + WAVE_BUFFER_SIZE) % WAVE_BUFFER_SIZE] : 0;
  canvas.printf("AC: %.2fG", currentValue);
  
  // グラフ枠線
  canvas.drawRect(graphLeft, graphTop, graphWidth, graphHeight, TFT_DARKGREY);
  
  // Y軸の範囲（0 ～ displayRangeG）
  float rangeG = ACCEL_CONFIG.displayRangeG;
  
  // 閾値線を描画（3つすべて）
  for (uint8_t i = 0; i < ACCEL_TRIGGER_DATA_COUNT; i++) {
    float threshold = ACCEL_TRIGGER_DATA[i].threshold;
    int16_t thresholdY = graphTop + graphHeight - (int16_t)(threshold / rangeG * graphHeight);
    
    // 範囲内のみ描画
    if (thresholdY >= graphTop && thresholdY <= graphTop + graphHeight) {
      uint16_t color;
      const char* label;
      if (i == 0) { color = TFT_YELLOW; label = "L"; }
      else if (i == 1) { color = TFT_ORANGE; label = "M"; }
      else { color = TFT_RED; label = "H"; }
      
      canvas.drawFastHLine(graphLeft, thresholdY, graphWidth, color);
      canvas.setTextColor(color);
      canvas.setCursor(graphLeft + graphWidth + 2, thresholdY - 3);
      canvas.print(label);
    }
  }
  
  // 波形を描画
  canvas.setTextColor(TFT_WHITE);
  float xScale = (float)graphWidth / WAVE_BUFFER_SIZE;
  
  for (uint16_t i = 1; i < WAVE_BUFFER_SIZE; i++) {
    uint16_t idx1 = (waveIndex + i - 1) % WAVE_BUFFER_SIZE;
    uint16_t idx2 = (waveIndex + i) % WAVE_BUFFER_SIZE;
    
    // Y座標計算（下が0、上がrangeG）
    int16_t y1 = graphTop + graphHeight - (int16_t)(waveBuffer[idx1] / rangeG * graphHeight);
    int16_t y2 = graphTop + graphHeight - (int16_t)(waveBuffer[idx2] / rangeG * graphHeight);
    
    // 範囲内に収める
    y1 = constrain(y1, graphTop, graphTop + graphHeight);
    y2 = constrain(y2, graphTop, graphTop + graphHeight);
    
    int16_t x1 = graphLeft + (int16_t)((i - 1) * xScale);
    int16_t x2 = graphLeft + (int16_t)(i * xScale);
    
    canvas.drawLine(x1, y1, x2, y2, TFT_GREEN);
  }
  
  // 送信履歴表示（下部）
  if (lastSentTime > 0) {
    canvas.setCursor(2, screenH - 12);
    canvas.setTextColor(TFT_CYAN);
    
    // 経過時間を計算
    uint32_t elapsed = (millis() - lastSentTime) / 1000;
    if (elapsed < 60) {
      canvas.printf("Sent: %s P:%d (%lus ago)", lastSentLevel, lastSentPower, elapsed);
    } else {
      canvas.printf("Sent: %s P:%d (%lum ago)", lastSentLevel, lastSentPower, elapsed / 60);
    }
  }
  
  // スプライトを画面に転送
  canvas.pushSprite(0, 0);
}

// 初期化
void init() {
  // 画面を横向きに設定
  M5.Display.setRotation(1);  // 横向き（90度回転）
  
  // 乱数シード初期化
  randomSeed(esp_random());
  
  // IMUが見つからない場合は何もしない（M5.begin()後に呼び出す）
  auto imu_type = M5.Imu.getType();
  if (imu_type == m5::imu_none) {
    Serial.println("AccelTrigger: IMU not found!");
    return;
  }
  
  const char* imuName;
  switch (imu_type) {
    case m5::imu_mpu6886: imuName = "MPU6886"; break;
    case m5::imu_bmi270: imuName = "BMI270"; break;
    default: imuName = "Unknown"; break;
  }
  Serial.printf("AccelTrigger: IMU found (%s)\n", imuName);
  
  // 波形バッファを初期化
  memset(waveBuffer, 0, sizeof(waveBuffer));
  memset(acBuffer, 0, sizeof(acBuffer));
  
  // NVSからキャリブレーション読み込み
  if (M5.Imu.loadOffsetFromNVS()) {
    Serial.println("AccelTrigger: Loaded calibration from NVS");
  }
  
  // ダブルバッファリング用スプライト作成
  int16_t w = M5.Display.width();
  int16_t h = M5.Display.height();
  canvas.createSprite(w, h);
  canvas.setTextSize(1);
  canvasInitialized = true;
  Serial.printf("AccelTrigger: Canvas created (%dx%d)\n", w, h);
  
  Serial.printf("AccelTrigger: Axis=%d, MinThreshold=%.2fG, DeadTime=%dms, AcSamples=%d\n",
                (int)ACCEL_CONFIG.axis, ACCEL_TRIGGER_DATA[0].threshold, 
                ACCEL_CONFIG.deadTimeMs, ACCEL_CONFIG.acSampleCount);
}

// メインループ処理
void loop() {
  uint32_t now = millis();
  
  // サンプリング間隔チェック
  if (now - lastSampleTime < ACCEL_CONFIG.sampleIntervalMs) {
    return;
  }
  lastSampleTime = now;
  
  // IMU更新
  if (!M5.Imu.update()) {
    return;
  }
  
  auto data = M5.Imu.getImuData();
  
  // 生の加速度値を取得
  float rawValue = getRawAccelValue(data.accel.x, data.accel.y, data.accel.z);
  
  // AC成分を計算（絶対値）
  float acValue = updateAcValue(rawValue);
  
  // 波形バッファに追加（AC値）
  waveBuffer[waveIndex] = acValue;
  waveIndex = (waveIndex + 1) % WAVE_BUFFER_SIZE;
  
  // 閾値判定（デッドタイム考慮）- 最小閾値はACCEL_TRIGGER_DATA[0]から取得
  if (acValue >= ACCEL_TRIGGER_DATA[0].threshold) {
    if (now - lastTriggerTime >= ACCEL_CONFIG.deadTimeMs) {
      lastTriggerTime = now;
      triggerCount++;
      
      // 閾値に応じたデータを取得
      const char* levelName;
      uint8_t power;
      uint8_t levelIndex;
      const uint8_t* baseData = getTriggerDataWithPower(acValue, &levelName, &power, &levelIndex);
      
      if (baseData != nullptr) {
        // 送信データを構築
        uint8_t sendData[8];
        memcpy(sendData, baseData, 8);
        
        // subidをランダムに設定
        sendData[4] = random(0, ACCEL_CONFIG.subIdMax + 1);
        
        // パワーを設定（左右同じ）
        sendData[5] = power;
        sendData[6] = power;
        
        // 送信
        espnowManager::sendData(sendData, 8);
        
        // 送信履歴を記録
        strncpy(lastSentLevel, levelName, sizeof(lastSentLevel) - 1);
        lastSentPower = power;
        lastSentTime = now;
        
        Serial.printf("AccelTrigger: AC=%.2fG [%s] SubID=%d Power=%d -> Sent! (count=%lu)\n", 
                      acValue, levelName, sendData[4], power, triggerCount);
      }
    }
  }
  
  // 波形表示（ダブルバッファリングなのでちらつかない）
  static uint32_t lastDrawTime = 0;
  if (ACCEL_CONFIG.displayEnabled && (now - lastDrawTime >= 50)) {  // 20fps
    lastDrawTime = now;
    drawWaveform();
  }
}

// トリガーカウントを取得
uint32_t getTriggerCount() {
  return triggerCount;
}

// トリガーカウントをリセット
void resetTriggerCount() {
  triggerCount = 0;
}

}  // namespace AccelTrigger

#endif  // ENABLE_ACCEL_TRIGGER
