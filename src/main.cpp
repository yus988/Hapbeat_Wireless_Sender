#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef ENABLE_COLOR_SENSOR
  #include <ColorSensor.h>
  #include <FastLED.h>
#endif

#ifndef NO_DISPLAY
  #include <M5UImanager.h>
#endif

#ifdef ESPNOW
  #include <espnow_manager.h>
#elif MQTT
  #include <MQTT_manager.h>
#endif

#include "adjustmentParams.h"
#include "pinAssign.h"
#include "config.h"

const char* cmd_stat;
const char* cmd_btn;
const char* cmd_conn;

String lastColor = "None";

CRGB _leds[1];

void mqttStatusCallback(const char* status) {
  // Serial.println(status);
  // if (strcmp(status, "Successfully connected to Hapbeat") == 0) {
  //   _leds[0] = CREATE_CRGB(COLOR_CONNECTED);
  //   Serial.println("turn LED to GREEN");
  // } else if (strstr(status, "failed") != NULL) {
  //   _leds[0] = CREATE_CRGB(COLOR_UNCONNECTED);
  // }
  // FastLED.show();
}

TaskHandle_t thp[2];
void TaskColorSensor(void* args) {
  static uint8_t r, g, b;
  while (1) {
    ColorSensor::loopColorSensor();
    ColorSensor::getColorValues(r, g, b);

    String color = ColorSensor::determineColor(r, g, b);
    Serial.printf("color is: ");
    Serial.println(color);
    // "None"から他の色に変わった場合に、Hapbeatにメッセージを送信
    // 色が変わった場合にメッセージを送信
    if (color != lastColor) {
      if (lastColor == "None" || color != "None") {
        // 送信するメッセージの生成
        int id, lVol;
        if (color == "Red") {
          id = RED_PARAMS.id;
          lVol = RED_PARAMS.vol;
        } else if (color == "Green") {
          id = GREEN_PARAMS.id;
          lVol = GREEN_PARAMS.vol;
        } else if (color == "Yellow") {
          id = YELLOW_PARAMS.id;
          lVol = YELLOW_PARAMS.vol;
        }
        int rVol = lVol;
        char message[100];
        snprintf(message, sizeof(message), "%d,%d,%d,%d,%d,%d,%d,%d", CATEGORY,
                 WEARER_ID, DEVICE_POS, id, SUB_ID, lVol, rVol, PLAY_CMD);
        // 全てのHapbeatから応答があるまで while
        // でメッセージを送り続ける処理を入れる
        MQTT_manager::sendMessageToHapbeat(message);
      }
      lastColor = color;
    }

    StaticJsonDocument<200> doc;
    doc["r"] = r;
    doc["g"] = g;
    doc["b"] = b;
    String jsonMessage;
    serializeJson(doc, jsonMessage);

    MQTT_manager::sendMessageToWebApp(jsonMessage.c_str());
    delay(10000);
  }
}

void TaskMQTT(void* args) {
  while (1) {
    MQTT_manager::loopMQTTclient();
    if (MQTT_manager::mqttConnected) {
      _leds[0] = CREATE_CRGB(COLOR_CONNECTED);
    } else {
      _leds[0] = CREATE_CRGB(COLOR_UNCONNECTED);
    }
    FastLED.show();
    delay(100);
  }
}

void setup(void) {
  Serial.begin(115200);

#if defined(ENABLE_DISPLAY)
  initM5UImanager();
#endif

#ifdef ENABLE_COLOR_SENSOR
  FastLED.addLeds<NEOPIXEL, LED_PIN>(_leds, 1);
  _leds[0] = CREATE_CRGB(COLOR_UNCONNECTED);
  FastLED.show();
  ColorSensor::initColorSensor();
  xTaskCreatePinnedToCore(TaskColorSensor, "TaskColorSensor", 8192, NULL, 10,
                          &thp[1], 1);
#endif

#ifdef ESPNOW
  initEspNow();
#elif MQTT
  MQTT_manager::initMQTTclient(mqttStatusCallback);
  xTaskCreatePinnedToCore(TaskMQTT, "TaskMQTT", 8192, NULL, 23, &thp[0], 1);
#endif
}

void loop(void) {
#ifdef ESPNOW
  sendSerialViaESPNOW();
#endif
#if defined(ENABLE_DISPLAY)
  cmd_stat = "empty";
  cmd_btn = M5ButtonNotify(cmd_stat);
  if (cmd_btn != "empty") {
    SendEspNOW(cmd_btn);
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
