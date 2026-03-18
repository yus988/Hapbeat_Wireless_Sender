#include <Arduino.h>
// XIAO ESP32C3 等、M5Unified を使わないenvでもビルドできるようにする
#if __has_include(<M5Unified.h>)
  #include <M5Unified.h>
#endif
#include "globals.h"
TaskHandle_t thp[2];

#include <espnow_manager.h>
#ifdef ENABLE_AUDIO_STREAM
  #include <audioStreamSender.h>
#endif

#include "adjustParams.h"
#include "config.h"
#include "pinAssign.h"

#ifdef ENABLE_DISPLAY
  #include <M5UImanager.h>

const char* cmd_stat;
const char* cmd_btn;
const char* cmd_conn;
#endif

#ifdef ENABLE_COLOR_SENSOR
  #include <ColorSensor.h>
  #include <FastLED.h>
  #include <ArduinoJson.h>
String lastColor = "None";
CRGB _leds[1];

// 色の反映
String determineColor(uint8_t r, uint8_t g, uint8_t b) {
  if (r >= RED_THD.rMin && g <= RED_THD.gMax && b <= RED_THD.bMax) {
    return "Red";
  } else if (r <= BLUE_THD.rMax && g <= BLUE_THD.gMax && b >= BLUE_THD.bMin) {
    return "Blue";
  } else if (r >= YELLOW_THD.rMin && g >= YELLOW_THD.gMin &&
             b <= YELLOW_THD.bMax) {
    return "Yellow";
  } else {
    return "None";
  }
}

void TaskColorSensor(void* args) {
  static uint8_t r, g, b;
  static int count = 0;                     // 処理のカウント変数
  static unsigned long lastChangeTime = 0;  // 最後に色が変わった時間
  static String lastSentColor = "";         // 最後に送信した色

  static unsigned long lastNoneTime = 0;  // 最後にNoneが検出された開始時間

  static bool sentMessageForNone =
      false;  // Noneの色に対してメッセージを送ったかどうかのフラグ

  const int interval = SEND_WEBAPP_INTERVAL /
                       COLOR_SENSOR_INTERVAL;  // 任意の変数回ごとに実行する回数
  const unsigned long colorChangeDelay =
      COLOR_CHANGE_INTERVAL;  // 変数:
                              // 赤、青、黄以外の色が経過する時間（ミリ秒）

  while (1) {
    ColorSensor::getColorValues(r, g, b);
    String color = determineColor(r, g, b);
    Serial.printf("R: %d G: %d B: %d, color is: %s\n", r, g, b, color.c_str());

    unsigned long currentTime = millis();
    bool shouldSendMessage = false;

    // 色が変わった場合の処理
    if (color != lastColor) {
      // 赤、青、黄のいずれかの色が検出された場合
      if (color == "Red" || color == "Blue" || color == "Yellow") {
        sentMessageForNone = false;
        lastNoneTime = 0;
        if (lastSentColor != color ||
            (currentTime - lastChangeTime >= colorChangeDelay)) {
          // 色が変わった、または一定時間経過している場合に送信
          shouldSendMessage = true;
          lastSentColor = color;         // 最後に送信した色を更新
          lastChangeTime = currentTime;  // 色が変わった時間を記録
        }
      } else {
        // 赤、青、黄以外の色が検出された場合
        lastChangeTime = currentTime;  // 色が変わった時間を記録
        if (!sentMessageForNone) {
          lastNoneTime = currentTime;  // Noneが始まった時間を記録
        }
      }
    }

    // Noneが1分間連続しているか確認
    if (color == "None" && !sentMessageForNone &&
        (currentTime - lastNoneTime >= RETAIN_REFRESH_INTERVAL)) {
      // Noneの色が1分間続いたらメッセージを送信
      // MQTT削除により無効化
      sentMessageForNone = true;  // メッセージを送信したフラグを立てる
    }

    if (shouldSendMessage) {
      // 送信するメッセージの生成
      int id, lVol;
      if (color == "Red") {
        id = RED_PARAMS.id;
        lVol = RED_PARAMS.vol;
      } else if (color == "Blue") {
        id = BLUE_PARAMS.id;
        lVol = BLUE_PARAMS.vol;
      } else if (color == "Yellow") {
        id = YELLOW_PARAMS.id;
        lVol = YELLOW_PARAMS.vol;
      }
      int rVol = lVol;
      char message[100];
      snprintf(message, sizeof(message), "%d,%d,%d,%d,%d,%d,%d,%d", CATEGORY,
               WEARER_ID, DEVICE_POS, id, SUB_ID, lVol, rVol, PLAY_CMD);
      // MQTT削除により無効化
    }

  #if defined(INTERNET)
    count++;
    if (count >= interval) {
      // 任意の変数回ごとに実行する処理
      StaticJsonDocument<200> doc;
      doc["r"] = r;
      doc["g"] = g;
      doc["b"] = b;
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      // MQTT削除により無効化

      count = 0;  // カウントをリセット
    }
  #endif

    lastColor = color;
    delay(COLOR_SENSOR_INTERVAL);
  }
}

#endif

#ifdef ENABLE_ACCELOMETOR
  #include "AccelmImpactDetector.h"
#endif

#ifdef ENABLE_ACCEL_TRIGGER
  #include <AccelTrigger.h>
#endif

//////////////////// task //////////////////////

void setup(void) {
#if defined(ENABLE_AUDIO_STREAM) && defined(STREAM_SOURCE_SERIAL)
  Serial.setRxBufferSize(4096);
  Serial.begin(921600);
#else
  Serial.begin(115200);
#endif
  // USBSerial.begin(921600);

#ifdef REPEATER
  // 中継器専用セットアップ（最小限の初期化で低遅延重視）
  Serial.println("=== ESP-NOW Repeater Mode ===");

  // M5Unified基本初期化（ディスプレイに必要）
  auto cfg = M5.config();
  cfg.external_spk = false;
  M5.begin(cfg);

  #if defined(ENABLE_DISPLAY)
  // ディスプレイ初期化
  initM5UImanager();
  espnowManager::setBtnData(data_BtnA, data_BtnB, data_BtnC, 8);
  Serial.println("Display initialized for repeater");
  #endif

  espnowManager::initRepeaterMode();
  espnowManager::initEspNow();
  Serial.println("Repeater ready - low latency mode");
  return;  // 他の初期化をスキップ
#endif

#ifdef ENABLE_ACCEL_TRIGGER
  // M5StickC Plus2用の初期化（ENABLE_DISPLAYがあっても専用初期化を使用）
  Serial.println("=== AccelTrigger Mode (M5StickC Plus2) ===");
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  
  AccelTrigger::init();
  espnowManager::setBtnData(data_AccelLow, data_AccelMid, data_AccelHigh, 8);
  
#elif defined(ENABLE_DISPLAY)
  // 通常のディスプレイ初期化（M5Stack Basic等）
  initM5UImanager();
  espnowManager::setBtnData(data_BtnA, data_BtnB, data_BtnC, 8);
#endif

#ifdef ENABLE_COLOR_SENSOR
  FastLED.addLeds<NEOPIXEL, LED_PIN>(_leds, 1);
  _leds[0] = CREATE_CRGB(COLOR_UNCONNECTED);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.show();
  ColorSensor::initColorSensor();
  xTaskCreatePinnedToCore(TaskColorSensor, "TaskColorSensor", 8192, NULL, 10,
                          &thp[1], 1);

  // M5Capuleをバッテリー駆動で使いたいとき
  #ifdef M5CAPSULE
  pinMode(GPIO_NUM_46, OUTPUT);
  digitalWrite(GPIO_NUM_46, HIGH);
  #endif

#endif

#ifdef ENABLE_ACCELOMETOR
  if (!AccelmImpactDetector::initAccelm()) {
    Serial.println("Failed to initialize accelerometer!");
  }
#endif

  espnowManager::initEspNow();

#ifdef ENABLE_AUDIO_STREAM
  audioStreamSender::init();
  Serial.println("=== Audio Stream Mode ===");
#endif
}

void loop(void) {
#ifdef REPEATER
  delay(1);
  return;
#endif

#ifdef ENABLE_AUDIO_STREAM
  #ifdef STREAM_SOURCE_SERIAL
    audioStreamSender::processSerialStream();
  #endif
  #ifdef ENABLE_DISPLAY
    audioStreamSender::drawScopeIfEnabled();
  #endif
  return;
#endif

#ifdef ENABLE_TEST_DATA
  espnowManager::sendTestDataTick();
#endif

  espnowManager::sendSerialViaESPNOW();

#ifdef ENABLE_ACCELOMETOR
  AccelmImpactDetector::showAccelGraph();
  delay(1);
#endif

#ifdef ENABLE_ACCEL_TRIGGER
  M5.update();  // ボタン状態更新
  AccelTrigger::loop();
  
  // ボタンAでキャリブレーション
  if (M5.BtnA.wasPressed()) {
    Serial.println("AccelTrigger: Starting calibration...");
    M5.Imu.setCalibration(64, 64, 64);
  }

#elif defined(ENABLE_DISPLAY)
  // 通常のディスプレイモード（M5Stack Basic等）
  cmd_stat = "empty";
  cmd_btn = M5ButtonNotify(cmd_stat);
  if (cmd_btn != "empty") {
    espnowManager::SentEspnowTest(cmd_btn);
    Serial.println(cmd_btn);
  }
#endif
}

#if !defined(ARDUINO) && defined(ESP_PLATFORM)
extern "C" {
void loopTask(void*) {
  setup();
  for (;;) {
    loop();
  }
  vTaskDelete(NULL);
}

void app_main() {
  xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
}
}
#endif
