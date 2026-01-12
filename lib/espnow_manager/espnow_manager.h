#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <stdint.h>

namespace espnowManager {
void initEspNow(void);
void loopEspNowTask(void* pvParameters);
void sendSerialViaESPNOW(void);
void SentEspnowTest(const char* cmd);
void setBtnData(const uint8_t* dataA, const uint8_t* dataB,
                const uint8_t* dataC, size_t size);

// 汎用データ送信関数（加速度トリガー等で使用）
void sendData(const uint8_t* data, size_t size);

#ifdef ENABLE_TEST_DATA
void sendTestDataTick();
#endif

#ifdef REPEATER
void initRepeaterMode();
#endif
}  // namespace espnowManager

#endif
