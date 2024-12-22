#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

namespace espnowManager {
void initEspNow(void);
void sendSerialViaESPNOW(void);
void SentEspnowTest(const char* cmd);
void setBtnData(const uint8_t* dataA, const uint8_t* dataB,
                const uint8_t* dataC, size_t size);
}  // namespace espnowManager

#endif
