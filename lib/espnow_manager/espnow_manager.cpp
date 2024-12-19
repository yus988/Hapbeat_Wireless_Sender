#include <WiFi.h>
#include <esp_now.h>

#ifndef NO_DISPLAY
  #include <M5Unified.h>
#endif

namespace espnowManager {

esp_now_peer_info_t slave;
// data = [category, wearer_id, device_pos, data_id, sub_id, L_Vol, R_Vol,
// isLoop ] isLoop, 0=oneshot, 1=loopStart, 2=stop

#define ELEMENTS_NUM 8 /**< カンマ区切りデータの項目数 */
#define DATA_SIZE sizeof(uint8_t[ELEMENTS_NUM])
int sendTimes = 0;
unsigned int beginIndex;  // 要素の開始位置
static String elements[ELEMENTS_NUM];

const uint8_t data_empty[] = {0, 0, 0, 0, 0, 000, 000, 0};
const uint8_t data_BtnA[] = {0, 0, 99, 0, 0, 100, 100, 0};
const uint8_t data_BtnB[] = {0, 0, 99, 5, 0, 50, 50, 1};
const uint8_t data_BtnC[] = {0, 0, 99, 5, 0, 50, 50, 2};

void displayData(const uint8_t* data) {
  // 上部データ表示エリアをクリア（ボタンエリアを残す）
  int dataDisplayHeight = M5.Display.height() - 30;  // ボタン表示エリアを除外
  M5.Display.fillRect(0, 0, M5.Display.width(), dataDisplayHeight, TFT_BLACK);

  // データ表示設定
  M5.Display.setCursor(0, 10);  // 常に上部に表示
  M5.Display.setTextSize(2);

  String playtype_str = (data[7] == 0)   ? "oneshot"
                        : (data[7] == 1) ? "loop_start"
                        : (data[7] == 2) ? "loop_stop"
                                         : "bg_loop";

  // ラベルとデータの配列
  const char* labels[] = {"channel", "wearer",   "position", "sound_id",
                          "sub_id",  "volume_L", "volume_R", "playtype"};

  // 行間スペース設定
  int lineSpacing = 2;

  // 送信回数の表示
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.printf("Send Times: ");
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.printf("%d\n", sendTimes);
  M5.Display.setCursor(0, M5.Display.getCursorY() + lineSpacing);

  // Data 配列の表示
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.print("{");
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    M5.Display.printf("%d", data[i]);
    if (i < ELEMENTS_NUM - 1) {
      M5.Display.print(",");
    }
  }
  M5.Display.println("}");
  M5.Display.setCursor(0, M5.Display.getCursorY() + lineSpacing);

  // 各データ項目の表示
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%-9s= ", labels[i]);  // ラベルを左揃えで表示
    M5.Display.setTextColor(TFT_YELLOW);
    if (i == 7) {
      M5.Display.printf("%d (%s)\n", data[i], playtype_str.c_str());
    } else {
      M5.Display.printf("%d\n", data[i]);
    }
    M5.Display.setCursor(0, M5.Display.getCursorY() + lineSpacing);
  }

  M5.update();
}

// 送信コールバック
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.printf("Last Packet Sent to: %s\n", macStr);
  Serial.printf("Last Packet Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS
                                                     ? "Delivery Success"
                                                     : "Delivery Fail");
}

void SentEspnowTest(const char* cmd) {
  const uint8_t* data;

  if (strcmp(cmd, "BtnA") == 0) {
    data = data_BtnA;
  } else if (strcmp(cmd, "BtnB") == 0) {
    data = data_BtnB;
  } else if (strcmp(cmd, "BtnC") == 0) {
    data = data_BtnC;
  } else {
    data = data_empty;
  }

  esp_now_send(slave.peer_addr, data, DATA_SIZE);
  displayData(data);
}

void initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = 0xff;
  }
  slave.channel = 0;
  slave.encrypt = false;
  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK) {
    Serial.println("Pair success");
  } else {
    Serial.println("Pair failed");
  }
  esp_now_register_send_cb(OnDataSent);
}

/*
// [category, wearer, pos, id, subid, L_Vol, R_Vol]
category = 大枠のチャンネル（ディスプレイに表示されるチャンネル）
wearer = 装着者のID（複数プレイヤーで個別に刺激を変えたい場合）
pos = 装着位置（各Hapbeatごとに設定 or ボタン操作で切り替え
id = 音声の種類（銃撃、ダメージなど）
subid = 同じ音声種類内での微小差分（連続するイベントでちょっとずつ変えたい場合）
L_Vol = 左側の振動強度
R_Vol = 右側の振動強度
ex "0,0,0,0,100,100"
*/

void sendSerialViaESPNOW(void) {
  if (Serial.available() > 0) {
    // 終了文字まで取得
    String str = Serial.readStringUntil('\n');
    beginIndex = 0;
    for (uint8_t i = 0; i < ELEMENTS_NUM; i++) {
      if (i != (ELEMENTS_NUM - 1)) {
        uint8_t endIndex;
        endIndex = str.indexOf(',', beginIndex);
        // カンマが見つかった場合
        if (endIndex != -1) {
          elements[i] = str.substring(beginIndex, endIndex);
          beginIndex = endIndex + 1;
        } else {
          break;
        }
      } else {
        elements[i] = str.substring(beginIndex);
      }
    }
    uint8_t data[ELEMENTS_NUM];
    for (uint8_t i = 0; i < ELEMENTS_NUM; i++) {
      data[i] = elements[i].toInt();
    }
    esp_now_send(slave.peer_addr, data, sizeof(data));

#if defined(ENABLE_DISPLAY)
    displayData(data);
    sendTimes += 1;
#endif
  }
}

}  // namespace espnowManager