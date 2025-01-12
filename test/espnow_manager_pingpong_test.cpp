#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace espnowManager {

#define ELEMENTS_NUM 8  // データの項目数

esp_now_peer_info_t slave;

// ★ データ受信コールバック（Ping-Pong測定用）
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  unsigned long recvTime = micros();  // ★ 受信時刻
  unsigned long sentTime;

  // ★ 送信側のタイムスタンプを取得
  memcpy(&sentTime, incomingData, sizeof(unsigned long));

  // ★ 往復遅延（RTT）と片道遅延の計算
  unsigned long roundTripTime = recvTime - sentTime;
  unsigned long oneWayDelay = roundTripTime / 2;

  // ★ シリアル出力で遅延を表示
  Serial.printf("★ Ping-Pong片道遅延: %lu us\n", oneWayDelay);
}

// ★ データ送信（Ping送信）
void SentEspnowTest(const char* cmd) {
  static uint8_t data[ELEMENTS_NUM];
  unsigned long startTime = micros();  // ★ 送信直前のタイムスタンプ

  // ★ タイムスタンプのみ送信（ダミーデータ不要）
  memcpy(&data[0], &startTime, sizeof(unsigned long));

  // ★ データ送信
  esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
  if (result == ESP_OK) {
    Serial.println("Ping送信成功");
  } else {
    Serial.printf("Ping送信失敗: %d\n", result);
  }
}

// ★ ESP-NOW初期化（Ping-Pong測定用）
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

  esp_now_register_recv_cb(OnDataRecv);  // ★ 受信コールバック登録
}

}  // namespace espnowManager
