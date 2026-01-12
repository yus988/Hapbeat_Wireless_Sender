#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>        // ★ Wi-Fiの省電力設定用
#include <esp_idf_version.h>  // ★ バージョン判定
#ifdef ENABLE_DISPLAY
  #include <M5Unified.h>
#endif

namespace espnowManager {

namespace {

void configureRadioForRange() {
  // 最大全出力 (21 dBm)
  esp_wifi_set_max_tx_power(84);

  // 低レート優先で受信感度を確保
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L);
  esp_wifi_config_espnow_rate(WIFI_IF_AP, WIFI_PHY_RATE_1M_L);
#endif

  // 11b/g/n + ロングレンジを有効化
#if defined(WIFI_PROTOCOL_LR)
  const uint8_t protocolMask = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                               WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR;
#else
  const uint8_t protocolMask = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                               WIFI_PROTOCOL_11N;
#endif
  esp_wifi_set_protocol(WIFI_IF_STA, protocolMask);
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
}

}  // namespace

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

const uint8_t data_empty[] = {0, 0, 0, 0, 0, 0, 0, 0};
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

// 汎用データ送信関数（加速度トリガー等で使用）
void sendData(const uint8_t* data, size_t size) {
  if (size > ELEMENTS_NUM) size = ELEMENTS_NUM;
  esp_err_t result = esp_now_send(slave.peer_addr, data, size);
  sendTimes++;
  
#ifdef ENABLE_DEBUG
  if (result == ESP_OK) {
    Serial.println("ESP-NOW送信成功");
  } else {
    Serial.printf("ESP-NOW送信失敗: %d\n", result);
  }
#endif
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
#ifdef REPEATER
  // 中継器モード：3つ目の要素が50の場合のみ中継
  if (len == ELEMENTS_NUM) {
    // 3つ目の要素（position）が50かチェック
    if (incomingData[2] == 99) {
      static uint8_t relayData[ELEMENTS_NUM];

      // 受信データをコピー
      memcpy(relayData, incomingData, ELEMENTS_NUM);

      // 3つ目の要素（position）を99に変更
      relayData[2] = 50;

      // 即座に送信（低遅延重視）
      esp_now_send(slave.peer_addr, relayData, ELEMENTS_NUM);

      // 送信後に画面表示（遅延を避けるため送信完了後）
  #ifdef ENABLE_DISPLAY
      displayData(relayData);
      sendTimes += 1;
  #endif

  #ifdef ENABLE_DEBUG
      Serial.printf(
          "Relay: [%d,%d,%d,%d,%d,%d,%d,%d] -> position changed 99->50\n",
          relayData[0], relayData[1], relayData[2], relayData[3], relayData[4],
          relayData[5], relayData[6], relayData[7]);
  #endif
    } else {
  #ifdef ENABLE_DEBUG
      Serial.printf("Skip relay: position=%d (not 50)\n", incomingData[2]);
  #endif
    }
  }
#else
  // 既存のテスト用コード
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
#endif
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
  // 送信するデータ内容をシリアルに出力
  Serial.print("Sent data: ");
  for (int i = 0; i < ELEMENTS_NUM; ++i) {
    Serial.print(data[i]);
    if (i < ELEMENTS_NUM - 1) Serial.print(",");
  }
  Serial.println();
  displayData(data);
  esp_err_t result = esp_now_send(slave.peer_addr, data, ELEMENTS_NUM);
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

#ifdef ENABLE_TEST_DATA
void sendTestDataTick() {
  static unsigned long lastSend = 0;
  unsigned long now = millis();

  if (now - lastSend < 1000) {
    return;
  }
  lastSend = now;

#ifdef ENABLE_DISPLAY
  displayData(data_BtnA);
  sendTimes += 1;
#endif

  esp_err_t result = esp_now_send(slave.peer_addr, data_BtnA, ELEMENTS_NUM);

#ifdef ENABLE_DEBUG
  if (result == ESP_OK) {
    Serial.println("ESP-NOW送信成功 (TEST_DATA)");
  } else {
    Serial.printf("ESP-NOW送信失敗 (TEST_DATA): %d\n", result);
  }
#endif
}
#endif

uint8_t data[ELEMENTS_NUM];
volatile uint8_t receivedIndex = 0;
volatile bool dataReady = false;
esp_now_peer_info_t peer;

void initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);  // ★ Wi-Fiスリープ無効化

  configureRadioForRange();

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

#ifdef REPEATER
  Serial.println("ESP-NOW Repeater Mode Enabled");
  Serial.println("Waiting for data to relay...");
#endif

#ifdef ENABLE_DISPLAY
  displayData(data_empty);
#endif
}

#ifdef REPEATER
// 中継器専用の初期化関数
void initRepeaterMode() {
  // 中継器として最適化された設定
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);  // Wi-Fiスリープ完全無効化

  configureRadioForRange();

  Serial.println("Repeater mode optimized for low latency");
}
#endif

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
#ifdef ENABLE_DEBUG
      if (result == ESP_OK) {
        Serial.println("ESP-NOW送信成功");
      } else {
        Serial.printf("ESP-NOW送信失敗: %d\n", result);
      }
#endif

    } else if ((c >= '0' && c <= '9') || c == ',') {
      // ★ 数字とカンマだけをバッファに追加
      buffer[bufferIndex++] = c;
    }
  }
}

}  // namespace espnowManager