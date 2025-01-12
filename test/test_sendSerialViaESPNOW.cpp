

// // オーバーフロー対策あり 99us
// void sendSerialViaESPNOW(void) {
//   static char strBuffer[64];  // ★ 固定サイズのバッファ（動的確保回避）
//   static uint8_t data[ELEMENTS_NUM];
//   static uint8_t bufferIndex = 0;

//   while (Serial.available() > 0 && !testCompleted) {
//     unsigned long startReceiveTime = micros();  // ★ シリアル受信開始時間

//     char c = Serial.read();

//     if (c == '\n' || bufferIndex >= sizeof(strBuffer) - 1) {
//       strBuffer[bufferIndex] = '\0';  // ★ 文字列終端

//       unsigned long startParseTime = micros();  // ★ パース開始時間

//       // ★ データパース処理（高速化）
//       char *token = strtok(strBuffer, ",");
//       uint8_t i = 0;
//       while (token != NULL && i < ELEMENTS_NUM) {
//         data[i++] = atoi(token);
//         token = strtok(NULL, ",");
//       }

//       unsigned long startSendTime = micros();  // ★ ESP-NOW送信開始時間

//       // ★ ESP-NOW送信
//       esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));

//       unsigned long endSendTime = micros();  // ★ ESP-NOW送信完了時間

// #if defined(ENABLE_DISPLAY)
//       displayData(data);
//       sendTimes += 1;
// #endif

//       bufferIndex = 0;  // ★ バッファクリア

//       // ★ 処理時間の計測
//       unsigned long delayTime = endSendTime - startReceiveTime;
//       totalDelayTime += delayTime;
//       testCounter++;

//       // ★ 各回の遅延時間表示
//       Serial.printf("[%d回目] 処理時間: %lu us\n", testCounter, delayTime);

//       // ★ 100回到達で平均遅延時間を表示
//       if (testCounter >= TEST_COUNT && !testCompleted) {
//         unsigned long averageDelay = totalDelayTime / TEST_COUNT;

//         Serial.println("--------------------------------------------------");
//         Serial.printf("★ 100回の平均処理時間: %lu us\n", averageDelay);
//         Serial.println("--------------------------------------------------");

//         // ★ ディスプレイに結果を表示
//         M5.Lcd.fillScreen(BLACK);
//         M5.Lcd.setTextSize(2);
//         M5.Lcd.setCursor(10, 10);
//         M5.Lcd.println("★ テスト結果 ★");
//         M5.Lcd.printf("times: %d回\n", TEST_COUNT);
//         M5.Lcd.printf("average delay: %lu us\n", averageDelay);

//         testCompleted = true;
//       }

//       // ★ 送信結果の確認
//       if (result == ESP_OK) {
//         Serial.println("ESP-NOW送信成功");
//       } else {
//         Serial.printf("ESP-NOW送信失敗: %d\n", result);
//       }

//     } else {
//       strBuffer[bufferIndex++] = c;  // ★ シリアル受信データをバッファに追加
//     }
//   }
// }


// 106us
// void sendSerialViaESPNOW(void) {
//   static String str = "";
//   static uint8_t data[ELEMENTS_NUM];

//   while (Serial.available() > 0 && !testCompleted) {  // ★ テスト完了まで実行
//     unsigned long startReceiveTime = micros();  // ★ シリアル受信開始時間

//     char c = Serial.read();

//     if (c == '\n') {
//       unsigned long startParseTime = micros();  // ★ パース開始時間

//       // データパース処理
//       int startIdx = 0, endIdx = 0;
//       for (uint8_t i = 0; i < ELEMENTS_NUM; i++) {
//         endIdx = str.indexOf(',', startIdx);
//         data[i] = str.substring(startIdx, (endIdx == -1) ? str.length() : endIdx).toInt();
//         startIdx = endIdx + 1;
//       }

//       unsigned long startSendTime = micros();  // ★ ESP-NOW送信開始時間

//       esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));

//       unsigned long endSendTime = micros();  // ★ ESP-NOW送信完了時間

// #if defined(ENABLE_DISPLAY)
//       displayData(data);
//       sendTimes += 1;
// #endif

//       str = "";  // バッファクリア

//       // ★ 処理時間の計測
//       unsigned long delayTime = endSendTime - startReceiveTime;
//       totalDelayTime += delayTime;
//       testCounter++;

//       // ★ 各回の遅延時間表示
//       Serial.printf("[%d回目] 処理時間: %lu us\n", testCounter, delayTime);

//       // ★ 100回到達で平均遅延時間を表示
//       if (testCounter >= TEST_COUNT && !testCompleted) {
//         unsigned long averageDelay = totalDelayTime / TEST_COUNT;

//         Serial.println("--------------------------------------------------");
//         Serial.printf("★ 100回の平均処理時間: %lu us\n", averageDelay);
//         Serial.println("--------------------------------------------------");

//         // ★ ディスプレイに結果を表示
//         M5.Lcd.fillScreen(BLACK);             // 画面クリア
//         M5.Lcd.setTextSize(2);                // テキストサイズ設定
//         M5.Lcd.setCursor(10, 10);            // 表示位置
//         M5.Lcd.println("★ テスト結果 ★");
//         M5.Lcd.printf("times: %d回\n", TEST_COUNT);
//         M5.Lcd.printf("average delay: %lu us\n", averageDelay);

//         testCompleted = true;  // ★ テスト完了フラグを立てる
//       }

//       // ★ 送信結果の確認
//       if (result == ESP_OK) {
//         Serial.println("ESP-NOW送信成功");
//       } else {
//         Serial.printf("ESP-NOW送信失敗: %d\n", result);
//       }

//     } else {
//       str += c;
//     }
//   }
// }


// 596us
// void sendSerialViaESPNOW(void) {
//   if (Serial.available() > 0 && !testCompleted) {  // ★ テストが完了するまで実行
//     // ★ シリアル受信開始時のタイムスタンプ
//     unsigned long startReceiveTime = micros();

//     // ★ シリアルデータ受信（ブロッキング）
//     String str = Serial.readStringUntil('\n');

//     // ★ データパース開始のタイムスタンプ
//     unsigned long startParseTime = micros();

//     uint8_t data[ELEMENTS_NUM];
//     int beginIndex = 0;

//     // ★ データの分割と変換
//     for (uint8_t i = 0; i < ELEMENTS_NUM; i++) {
//       int endIndex = str.indexOf(',', beginIndex);
//       if (endIndex != -1) {
//         data[i] = str.substring(beginIndex, endIndex).toInt();
//         beginIndex = endIndex + 1;
//       } else {
//         data[i] = str.substring(beginIndex).toInt();
//         break;
//       }
//     }

//     // ★ ESP-NOW送信直前のタイムスタンプ
//     unsigned long startSendTime = micros();

//     // ★ ESP-NOW送信
//     esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));

//     // ★ ESP-NOW送信直後のタイムスタンプ
//     unsigned long endSendTime = micros();

// #if defined(ENABLE_DISPLAY)
//     displayData(data);
//     sendTimes += 1;
// #endif

//     // ★ 各処理時間の計算
//     unsigned long delayTime = endSendTime - startReceiveTime;
//     totalDelayTime += delayTime;
//     testCounter++;

//     // ★ 各回の遅延時間表示
//     Serial.printf("[%d回目] 処理時間: %lu us\n", testCounter, delayTime);

//     // ★ 100回到達で平均遅延時間を表示
//     if (testCounter >= TEST_COUNT && !testCompleted) {
//       unsigned long averageDelay = totalDelayTime / TEST_COUNT;
//       Serial.println("--------------------------------------------------");
//       Serial.printf("★ 100回の平均処理時間: %lu us\n", averageDelay);
//       Serial.println("--------------------------------------------------");

//       // ★ ディスプレイに結果を表示
//       M5.Lcd.fillScreen(BLACK);  // 画面クリア
//       M5.Lcd.setTextSize(2);     // テキストサイズ設定
//       M5.Lcd.setCursor(10, 10);  // 表示位置
//       M5.Lcd.println("★ テスト結果 ★");
//       M5.Lcd.printf("送信回数: %d回\n", TEST_COUNT);
//       M5.Lcd.printf("平均遅延: %lu us\n", averageDelay);

//       testCompleted = true;  // ★ テスト完了フラグを立てる
//     }

//     // ★ 送信結果の確認
//     if (result == ESP_OK) {
//       Serial.println("【改善前】ESP-NOW送信成功");
//     } else {
//       Serial.printf("【改善前】ESP-NOW送信失敗: %d\n", result);
//     }
//   }
// }