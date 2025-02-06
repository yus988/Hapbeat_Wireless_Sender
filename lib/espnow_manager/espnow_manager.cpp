#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>  // ★ Wi-Fiの省電力設定用
#ifdef ENABLE_DISPLAY
  #include <M5Unified.h>
#endif

namespace espnowManager {

// テスト用
#define TEST_COUNT 100             // テスト回数
unsigned long totalDelayTime = 0;  // 合計遅延時間
unsigned int testCounter = 0;      // 現在の送信回数
bool testCompleted = false;        // テスト完了フラグ

esp_now_peer_info_t slave;
// data = [category, wearer_id, device_pos, data_id, sub_id, L_Vol, R_Vol,
// isLoop ] isLoop, 0=oneshot, 1=loopStart, 2=stop
#define ELEMENTS_NUM 8 /**< カンマ区切りデータの項目数 */
#define DATA_SIZE sizeof(uint8_t[ELEMENTS_NUM])
int sendTimes = 0;
unsigned int beginIndex;  // 要素の開始位置
static String elements[ELEMENTS_NUM];

const uint8_t data_empty[] = {0, 0, 0, 0, 0, 000, 000, 0};
uint8_t data_BtnA[ELEMENTS_NUM] = {0};
uint8_t data_BtnB[ELEMENTS_NUM] = {0};
uint8_t data_BtnC[ELEMENTS_NUM] = {0};

// 全てのデータを一括で設定する関数
void setBtnData(const uint8_t* dataA, const uint8_t* dataB,
                const uint8_t* dataC, size_t size) {
  if (size == ELEMENTS_NUM) {
    memcpy(data_BtnA, dataA, size);
    memcpy(data_BtnB, dataB, size);
    memcpy(data_BtnC, dataC, size);
  }
}

///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////
#ifdef ENABLE_DISPLAY
void displayData(const uint8_t* data) {
  // 静的に前回のデータを保持
  static uint8_t previousData[ELEMENTS_NUM] = {0};
  static int previousSendTimes = -1;      // 前回の送信回数
  static String previousDataString = "";  // 前回の Data 表示内容
  static bool firstRun = true;            // 初回実行かどうかのフラグ

  // 表示位置設定
  int startY = 10;       // 最初の行の Y 座標
  int lineHeight = 20;   // 各行の高さ
  int labelWidth = 130;  // 項目名（ラベル）の幅

  // プレイタイプ文字列
  String playtype_str = (data[7] == 0)   ? "oneshot"
                        : (data[7] == 1) ? "loop_start"
                        : (data[7] == 2) ? "loop_stop"
                                         : "bg_loop";

  // ラベルとデータの配列
  const char* labels[] = {"category", "channel",  "position", "sound_id",
                          "sub_id",   "volume_L", "volume_R", "playtype"};

  // データ全体を文字列化
  String currentDataString = "{";
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    currentDataString += String(data[i]);
    if (i < ELEMENTS_NUM - 1) {
      currentDataString += ",";
    }
  }
  currentDataString += "}";

  // テキストサイズ設定
  M5.Display.setTextSize(2);

  // 送信回数の表示（変更がある場合のみ更新）
  if (sendTimes != previousSendTimes || firstRun) {
    M5.Display.fillRect(0, startY, M5.Display.width(), lineHeight,
                        TFT_BLACK);  // クリア
    M5.Display.setCursor(0, startY);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("Send Times: ");
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.printf("%d\n", sendTimes);
    previousSendTimes = sendTimes;
  }

  // Data の表示（変更がある場合のみ更新）
  if (currentDataString != previousDataString || firstRun) {
    int dataY = startY + lineHeight;
    M5.Display.fillRect(0, dataY, M5.Display.width(), lineHeight,
                        TFT_BLACK);  // クリア
    M5.Display.setCursor(0, dataY);
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.println(currentDataString);
    previousDataString = currentDataString;
  }

  // 各データ項目の描画
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    int yPos = startY + (i + 2) * lineHeight;  // 各項目の Y 座標

    // ラベルを常に描画
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor(0, yPos);
    M5.Display.printf("%-9s= ", labels[i]);  // ラベルを左揃えで表示

    // データ部分のみ変更があれば再描画
    if (data[i] != previousData[i] || firstRun) {
      // データのクリア領域を調整して `=` の右側をクリア
      M5.Display.fillRect(labelWidth, yPos, M5.Display.width() - labelWidth,
                          lineHeight, TFT_BLACK);
      M5.Display.setTextColor(TFT_YELLOW);
      M5.Display.setCursor(labelWidth, yPos);

      if (i == 7) {
        // playtype の特別表示
        M5.Display.printf("%d (%s)", data[i], playtype_str.c_str());
      } else {
        M5.Display.printf("%d", data[i]);
      }
    }
  }

  // 前回のデータを更新
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    previousData[i] = data[i];
  }

  firstRun = false;  // 初回実行フラグを無効化
}
#endif

/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// 送信コールバック
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  // Serial.printf("SendTime");
  // char macStr[18];
  // snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
  // mac_addr[0],
  //          mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.printf("Last Packet Sent to: %s\n", macStr);
  // Serial.printf("Last Packet Send Status: %s\n", status ==
  // ESP_NOW_SEND_SUCCESS
  //                                                    ? "Delivery Success"
  //                                                    : "Delivery Fail");
}

// テスト用
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  // Serial.printf("★pingpongrecieved\n ");

  unsigned long recvTime = micros();  // ★ 受信時刻
  unsigned long sentTime;

  // ★ 送信側のタイムスタンプを取得
  memcpy(&sentTime, incomingData, sizeof(unsigned long));

  // ★ 往復遅延（RTT）と片道遅延の計算
  unsigned long roundTripTime = recvTime - sentTime;
  unsigned long oneWayDelay = roundTripTime / 2;

  // ★ シリアル出力で遅延を表示
  // Serial.printf("★ Ping-Pong片道遅延: %lu us\n", oneWayDelay);
}

void SentEspnowTest(const char* cmd) {
#if defined(ENABLE_DISPLAY)

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
  displayData(data);
  esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
#endif

  // static uint8_t data[ELEMENTS_NUM];
  // unsigned long startTime = micros();  // ★ 送信直前のタイムスタンプ
  // // ★ タイムスタンプのみ送信（ダミーデータ不要）
  // memcpy(&data[0], &startTime, sizeof(unsigned long));
  // // ★ データ送信
  // esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
  // if (result == ESP_OK) {
  //   Serial.println("Ping送信成功");
  // } else {
  //   Serial.printf("Ping送信失敗: %d\n", result);
  // }
}

uint8_t data[ELEMENTS_NUM];
volatile uint8_t receivedIndex = 0;
volatile bool dataReady = false;
esp_now_peer_info_t peer;

void initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);  // ★ Wi-Fiスリープ無効化

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
  int wifi_ch = 1;
  esp_wifi_set_channel(wifi_ch, WIFI_SECOND_CHAN_NONE);  // ★ チャネル1に固定
  slave.channel = wifi_ch;
  slave.encrypt = false;
  esp_err_t addStatus = esp_now_add_peer(&slave);

  if (addStatus == ESP_OK) {
    Serial.println("Pair success");
  } else {
    Serial.println("Pair failed");
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
#ifdef ENABLE_DISPLAY
  displayData(data_empty);
#endif
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

// 固定化 115200 -> 81us, 921600 -> 81us
// ディスプレイ無効化 -> 71us
void sendSerialViaESPNOW(void) {
  static char buffer[32];  // ★ バッファ（最大32文字を想定）
  static uint8_t data[ELEMENTS_NUM];
  static uint8_t bufferIndex = 0;

  while (Serial.available() > 0 && !testCompleted) {
    unsigned long startReceiveTime = micros();  // ★ 受信開始時間

    char c = Serial.read();

    if (c == '\n' || bufferIndex >= sizeof(buffer) - 1) {
      buffer[bufferIndex] = '\0';  // ★ 文字列終端

      // ★ パース処理（カンマ区切りの8個の数値を解析）
      uint8_t dataIndex = 0;
      uint8_t value = 0;

      for (uint8_t i = 0; i <= bufferIndex; i++) {
        if (buffer[i] == ',' || buffer[i] == '\0') {
          data[dataIndex++] = value;
          value = 0;
          if (dataIndex >= ELEMENTS_NUM) break;  // ★ 8個読み取りで終了
        } else if (buffer[i] >= '0' && buffer[i] <= '9') {
          value = value * 10 + (buffer[i] - '0');  // ★ 数値変換
        }
      }

      unsigned long startSendTime = micros();  // ★ ESP-NOW送信開始

      // ★ ESP-NOW送信
      esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));

      unsigned long endSendTime = micros();  // ★ ESP-NOW送信完了

#if defined(ENABLE_DISPLAY)
      displayData(data);
      sendTimes += 1;
#endif

      bufferIndex = 0;  // ★ バッファクリア

      // ★ 処理時間の計測
      unsigned long delayTime = endSendTime - startReceiveTime;
      totalDelayTime += delayTime;
      testCounter++;

      // // ★ 各回の遅延時間表示
      // Serial.printf("[%d回目] 処理時間: %lu us\n", testCounter, delayTime);

      // // ★ 100回到達でディスプレイに平均遅延を表示
      // if (testCounter >= TEST_COUNT && !testCompleted) {
      //   unsigned long averageDelay = totalDelayTime / TEST_COUNT;

      //   Serial.println("--------------------------------------------------");
      //   Serial.printf("★ 100回の平均処理時間: %lu us\n", averageDelay);
      //   Serial.println("--------------------------------------------------");

      //   // ★ ディスプレイ表示
      //   M5.Lcd.fillScreen(BLACK);
      //   M5.Lcd.setTextSize(2);
      //   M5.Lcd.setCursor(10, 10);
      //   M5.Lcd.println("★ テスト結果 ★");
      //   M5.Lcd.printf("送信回数: %d回\n", TEST_COUNT);
      //   M5.Lcd.printf("平均遅延: %lu us\n", averageDelay);

      //   testCompleted = true;
      // }

      // ★ 送信結果の確認
      if (result == ESP_OK) {
        Serial.println("ESP-NOW送信成功");
      } else {
        Serial.printf("ESP-NOW送信失敗: %d\n", result);
      }

    } else if ((c >= '0' && c <= '9') || c == ',') {
      // ★ 数字とカンマだけをバッファに追加
      buffer[bufferIndex++] = c;
    }
  }
}

}  // namespace espnowManager